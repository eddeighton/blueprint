
#include "blueprint/edit.h"

#include "blueprint/blueprint.h"
#include "blueprint/basicarea.h"
#include "blueprint/spaceUtils.h"

#include "ed/ed.hpp"

#include "common/compose.hpp"
#include "common/assert_verify.hpp"

#include "wykobi.hpp"
#include "wykobi_algorithm.hpp"

#include <boost/numeric/conversion/bounds.hpp>

#include <algorithm>
#include <iomanip>

namespace Blueprint
{

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
const std::string& Blueprint::TypeName()
{
    static const std::string strTypeName( "blueprint" );
    return strTypeName;
}
Blueprint::Blueprint( const std::string& strName )
    :   Site( Ptr(), strName )
{

}

Blueprint::Blueprint( PtrCst pOriginal, Node::Ptr pNotUsed, const std::string& strName )
    :   Site( pOriginal, Ptr(), strName )
{
    ASSERT( !pNotUsed );
}

Node::Ptr Blueprint::copy( Node::Ptr pParent, const std::string& strName ) const
{   
    return Node::copy< Blueprint >( shared_from_this(), pParent, strName );
}

void Blueprint::init()
{
    Site::init();

    Connection::PtrVector connections, dudConnections;
    for_each( generics::collectIfConvert( connections, 
        Node::ConvertPtrType< Connection >(), Node::ConvertPtrType< Connection >() ) );
    for( Connection::PtrVector::iterator 
        i = connections.begin(),
        iEnd = connections.end(); i!=iEnd; ++i )
    {
        Connection::Ptr pConnection = *i;
        bool bAdded = false;

        Reference::Ptr pSourceRef = pConnection->get< Reference >( "source" );
        Reference::Ptr pTargetRef = pConnection->get< Reference >( "target" );
        if( pSourceRef && pTargetRef )
        {
            RefPtr pSource( pConnection, pSourceRef->getValue() );
            RefPtr pTarget( pConnection, pTargetRef->getValue() );
            
            Feature_ContourSegment::Ptr pSourceSegment = pSource.get< Feature_ContourSegment >();
            Feature_ContourSegment::Ptr pTargetSegment = pTarget.get< Feature_ContourSegment >();
            if( pSourceSegment && pTargetSegment )
            {
                Feature_Contour::Ptr pSourceContour = boost::dynamic_pointer_cast< Feature_Contour >( pSourceSegment->Node::getParent() );
                Feature_Contour::Ptr pTargetContour = boost::dynamic_pointer_cast< Feature_Contour >( pTargetSegment->Node::getParent() );
                if( pSourceContour && pTargetContour )
                {
                    Area::Ptr pSourceArea = boost::dynamic_pointer_cast< Area >( pSourceContour->Node::getParent() );
                    Area::Ptr pTargetArea = boost::dynamic_pointer_cast< Area >( pTargetContour->Node::getParent() );
                    if( pSourceArea && pTargetArea )
                    {
                        AreaBoundaryPair src( pSourceArea, pSourceSegment ),
                                        target( pTargetArea, pTargetSegment );
                        ConnectionPairing pairing( src, target );
                        if( m_connections.insert( std::make_pair( pairing, pConnection ) ).second )
                            bAdded = true;
                    }
                }
            }
        }
        if( ! bAdded ) dudConnections.push_back( pConnection );
    }

    for( Connection::Ptr pConnection : dudConnections )
    {
        remove( pConnection );
    }
}


bool Blueprint::add( Node::Ptr pNewNode )
{
    return Site::add( pNewNode );
}

void Blueprint::remove( Node::Ptr pNode )
{
    if( Connection::Ptr pConnection = boost::dynamic_pointer_cast< Connection >( pNode ) )
        generics::eraseAllSecond( m_connections, pConnection );
    Site::remove( pNode );
}

void Blueprint::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );
}

void Blueprint::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Site::save( node );
}

bool detectConnection( Feature_ContourSegment::Ptr p1, Feature_ContourSegment::Ptr p2, float& fDist )
{
    bool bValid = false;

    fDist = wykobi::lay_distance( 
        getContourSegmentPointAbsolute( p1, Feature_ContourSegment::eMidPoint ),
        getContourSegmentPointAbsolute( p2, Feature_ContourSegment::eMidPoint ));

    if( fDist < 32.0f )
        bValid = true;

    return bValid;
}

void Blueprint::computePairings( ConnectionPairingSet& pairings ) const
{
    //LOG_PROFILE_BEGIN( BlueprintPairingCalc );

    typedef std::multimap< float, ConnectionPairing > PairingDistanceMap;
    PairingDistanceMap distanceMapping;
    for( Site::PtrVector::const_iterator 
        i = m_spaces.begin(),
        iEnd = m_spaces.end(); i!=iEnd; ++i )
    {
        //get the blueprint boundaries
        if( Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( *i ) )
        {
            const Area::ContourPointVector& boundaries = pArea->getBoundaries();
            for( Area::ContourPointVector::const_iterator 
                j = boundaries.begin(),
                jEnd = boundaries.end(); j!=jEnd; ++j )
            {
                AreaBoundaryPair ab( pArea, *j );
                for( Site::PtrVector::const_iterator k = i+1u; k!=iEnd; ++k )
                {
                    if( Area::Ptr pAreaCmp = boost::dynamic_pointer_cast< Area >( *k ) )
                    {
                        const Area::ContourPointVector& boundaries = pAreaCmp->getBoundaries();
                        for( Area::ContourPointVector::const_iterator l = boundaries.begin(),
                            lEnd = boundaries.end(); l!=lEnd; ++l )
                        {
                            AreaBoundaryPair abCmp( pAreaCmp, *l );

                            //determine suitability of connection and distance
                            float fDist = 0.0f;
                            if( detectConnection( *j, *l, fDist ) )
                            {
                                distanceMapping.insert( std::make_pair( fDist, ConnectionPairing( ab, abCmp ) ) );
                            }
                        }
                    }
                }
            }
        }
    }
    
    typedef std::set< AreaBoundaryPair > AreaBoundaryPairSet;
    AreaBoundaryPairSet pairedUpSet;
    for( PairingDistanceMap::iterator 
        i = distanceMapping.begin(),
        iEnd = distanceMapping.end(); i!=iEnd; ++i )
    {
        if( pairedUpSet.find( i->second.first ) == pairedUpSet.end() && 
            pairedUpSet.find( i->second.second ) == pairedUpSet.end() )
        {
            pairedUpSet.insert( i->second.first );
            pairedUpSet.insert( i->second.second );
            pairings.insert( i->second );
        }
    }

    //LOG_PROFILE_END( BlueprintPairingCalc );
}

Blueprint::EvaluationResult Blueprint::evaluate( const EvaluationMode& mode, DataBitmap& data )
{
    ConnectionPairingSet pairings;
    computePairings( pairings );
    
    //LOG_PROFILE_BEGIN( BlueprintPairingMatch );

    ConnectionMap removals;
    ConnectionPairingVector additions;
    generics::match( m_connections.begin(), m_connections.end(), pairings.begin(), pairings.end(), 
        CompareConnectionIters(), 
        generics::collect( removals, generics::deref< ConnectionMap::const_iterator >() ),
        generics::collect( additions, generics::deref< ConnectionPairingSet::const_iterator >() ) );
        
    for( ConnectionMap::iterator i = removals.begin(),
        iEnd = removals.end(); i!=iEnd; ++i )
        remove( i->second );

    for( ConnectionPairingVector::iterator i = additions.begin(),
        iEnd = additions.end(); i!=iEnd; ++i )
    {
        Connection::Ptr pNewConnection( 
            new Connection( shared_from_this(), generateNewNodeName( "connection" ) ) );

        //determine the source and target references
        Ed::Reference sourceRef, targetRef;
        const bool b1 = calculateReference( pNewConnection, i->first.second, sourceRef );
        const bool b2 = calculateReference( pNewConnection, i->second.second, targetRef );
        ASSERT( b1 && b2 );
        pNewConnection->init( sourceRef, targetRef );
        add( pNewConnection );
        m_connections.insert( std::make_pair( *i, pNewConnection ) );
    }

    //LOG_PROFILE_END( BlueprintPairingMatch );

    if( !removals.empty() || !additions.empty() )
    {
        //LOG( info ) << "blueprint updated: " << removals.size() << 
        //    " removals : " << additions.size() << " additions.";
    }
    
    //LOG_PROFILE_BEGIN( blueprint_evaluation );

    EvaluationResult result;

    Site::PtrVector evaluated;
    Site::PtrList pending( m_spaces.begin(), m_spaces.end() );
    bool bEvaluated = true;
    while( bEvaluated )
    {
        bEvaluated = false;

        for( Site::PtrList::iterator 
            i = pending.begin(),
            iEnd = pending.end(); i!=iEnd; ++i )
        {
            Site::Ptr pFront = *i;
            if( pFront->canEvaluate( evaluated ) )
            {
                pFront->evaluate( mode, data );
                pending.erase( i );
                evaluated.push_back( pFront );
                bEvaluated = true;
                break;
            }
        }
    }

    if( pending.empty() )
        result.bSuccess = true;

    //LOG_PROFILE_END( blueprint_evaluation );


    return result;
}




}