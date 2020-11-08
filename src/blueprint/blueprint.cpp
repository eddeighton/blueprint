
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


namespace
{
    struct Point
    {
        typedef boost::shared_ptr< Point > Ptr;
        typedef std::vector< Ptr > Contour;
        typedef std::vector< Contour > Contours;
        float x,y;
        unsigned int uiIndex;
        unsigned int uiPointIndex;
        Feature_ContourSegment::Ptr pBoundary;
        Area::Ptr pArea;
        const Contour* pContour;
    };

    struct BoundaryPoint
    {
        unsigned int uiIndex = 0;
        float x = 0.0f, y = 0.0f;
        float fRatio = 0.0f;
        Feature_ContourSegment::Ptr pBoundary;
        BoundaryPoint( Feature_ContourSegment::Ptr _pBoundary ) : pBoundary( _pBoundary ) {}
    };
    
    typedef std::map< std::pair< float, float >, Point::Ptr > PointMap;
    
    void constructContour( Point::Contour& contour, ::Blueprint::Area::Ptr pArea, unsigned int& uiPointIndex, PointMap& points )
    {
        ::Blueprint::Site::FloatPairVector absoluteContour;
        pArea->getAbsoluteContour( absoluteContour );
        for( auto& p : absoluteContour )
        {
            Point::Ptr pPoint( new Point{    
                        p.first, 
                        p.second, 
                        0U, 
                        uiPointIndex++, 
                        Feature_ContourSegment::Ptr(), 
                        pArea, 
                        nullptr } );
            contour.push_back( pPoint );
            points.insert( std::make_pair( std::make_pair( pPoint->x, pPoint->y ), pPoint ) );
        }
    }

    typedef std::map< std::pair< unsigned int, float >, BoundaryPoint > BoundaryPointMap;
}



void Blueprint::getAbsoluteContours( std::vector< Site::FloatPairVector >& blueprintContours )
{
    Point::Contours areaContours;
    Point::Contours boundaryContours;

    unsigned int uiPointIndex = 0u;

    PointMap points;

    const Node::PtrVector& areas = getChildren();
    for( Node::PtrVector::const_iterator 
        i = areas.begin(),
        iEnd = areas.end(); i!=iEnd; ++i )
    {
        if( Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( *i ) )
        {
            Point::Contour c;
            constructContour( c, pArea, uiPointIndex, points );

            //add the internal contours
            for( Node::PtrVector::const_iterator 
                j = pArea->getChildren().begin(),
                jEnd = pArea->getChildren().end(); j!=jEnd; ++j )
            {
                if( Area::Ptr pInnerArea = boost::dynamic_pointer_cast< Area >( *j ) )
                {
                    Point::Contour c;
                    constructContour( c, pInnerArea, uiPointIndex, points );
                    std::reverse( c.begin(), c.end() );
                    areaContours.push_back( c );
                }
            }

            ///////////////////////////////////////////
            BoundaryPointMap boundaryPoints;
            {
                Feature_Contour::Ptr pContour = pArea->getContour();
                for( Node::PtrVector::const_iterator 
                    j = pContour->getChildren().begin(),
                    jEnd = pContour->getChildren().end(); j!=jEnd; ++j )
                {
                    if( Feature_ContourSegment::Ptr pBoundaries = 
                        boost::dynamic_pointer_cast< Feature_ContourSegment >( *j ) )
                    {
                        //determine if the boundary is within a connection
                        bool bHasConnection = false;
                        for( Node::PtrVector::const_iterator 
                            ib = areas.begin(),
                            ibEnd = areas.end(); ib!=ibEnd; ++ib )
                        {
                            if( Connection::Ptr pConnection = boost::dynamic_pointer_cast< Connection >( *ib ) )
                            {
                                RefPtr pSource( pConnection, pConnection->getSource()->getValue() );
                                RefPtr pTarget( pConnection, pConnection->getTarget()->getValue() );
                                Feature_ContourSegment::Ptr pSourceSegment = pSource.get< Feature_ContourSegment >();
                                Feature_ContourSegment::Ptr pTargetSegment = pTarget.get< Feature_ContourSegment >();
                                if( pSourceSegment == pBoundaries || pTargetSegment == pBoundaries )
                                {
                                    bHasConnection = true;
                                    break;
                                }
                            }
                        }

                        if( bHasConnection )
                        {
                            {
                                BoundaryPoint left( pBoundaries );
                                pBoundaries->getBoundaryPoint( Feature_ContourSegment::eLeft, left.uiIndex, left.x, left.y, left.fRatio );
                                left.x += pContour->getX( 0u );
                                left.y += pContour->getY( 0u );
                                pArea->getTransform().transform( left.x, left.y );
                                boundaryPoints.insert( std::make_pair( std::make_pair( left.uiIndex, left.fRatio ), left ) );
                            }
                            
                            {
                                BoundaryPoint right( pBoundaries );
                                pBoundaries->getBoundaryPoint( Feature_ContourSegment::eRight, right.uiIndex, right.x, right.y, right.fRatio );
                                right.x += pContour->getX( 0u );
                                right.y += pContour->getY( 0u );
                                pArea->getTransform().transform( right.x, right.y );
                                boundaryPoints.insert( std::make_pair( std::make_pair( right.uiIndex, right.fRatio ), right ) );
                            }
                        }
                    }
                }
            }
            
            unsigned int uiPolyIndex = 0u;
            for( BoundaryPointMap::reverse_iterator 
                j = boundaryPoints.rbegin(),
                jEnd = boundaryPoints.rend(); j!=jEnd; ++j, ++uiPolyIndex )
            {
                Point p{ j->second.x, j->second.y, 0U, uiPointIndex++, j->second.pBoundary, pArea, nullptr };
                Point::Ptr pPoint( new Point( p ) );
                c.insert( c.begin() + j->second.uiIndex + 1, pPoint );
                points.insert( std::make_pair( std::make_pair( j->second.x, j->second.y ), pPoint ) );
            }

            areaContours.push_back( c );
        }
    }
    
    for( Node::PtrVector::const_iterator 
        i = areas.begin(),
        iEnd = areas.end(); i!=iEnd; ++i )
    {
        if( Connection::Ptr pConnection = boost::dynamic_pointer_cast< Connection >( *i ) )
        {
            RefPtr pSource( pConnection, pConnection->getSource()->getValue() );
            RefPtr pTarget( pConnection, pConnection->getTarget()->getValue() );
            
            Feature_ContourSegment::Ptr pSourceSegment = pSource.get< Feature_ContourSegment >();
            Feature_ContourSegment::Ptr pTargetSegment = pTarget.get< Feature_ContourSegment >();
            VERIFY_RTE( pSourceSegment && pTargetSegment );
            if( pSourceSegment && pTargetSegment )
            {
                std::vector< wykobi::point2d< float > > wyPoints, convexHullPoints;
                wyPoints.push_back( getContourSegmentPointAbsolute( pSourceSegment, Feature_ContourSegment::eLeft ) );
                wyPoints.push_back( getContourSegmentPointAbsolute( pSourceSegment, Feature_ContourSegment::eRight ) );
                wyPoints.push_back( getContourSegmentPointAbsolute( pTargetSegment, Feature_ContourSegment::eLeft ) );
                wyPoints.push_back( getContourSegmentPointAbsolute( pTargetSegment, Feature_ContourSegment::eRight ) );

                wykobi::algorithm::convex_hull_graham_scan< wykobi::point2d< float > >( 
                    wyPoints.begin(), wyPoints.end(), std::back_inserter( convexHullPoints ) );

                std::reverse( convexHullPoints.begin(), convexHullPoints.end() );

                Point::Contour boundary;
                for( auto j = convexHullPoints.begin(),
                    jEnd = convexHullPoints.end(); j!=jEnd; ++j )
                {
                    const wykobi::point2d< float > p = *j;
                    PointMap::const_iterator iFind = points.find( std::make_pair( p.x, p.y ) );
                    VERIFY_RTE( iFind != points.end() );
                    Point::Ptr pPoint = iFind->second;
                    boundary.push_back( pPoint );
                }
                boundaryContours.push_back( boundary );
            }
        }
    }
    
    for( auto i = areaContours.begin(), 
        iEnd = areaContours.end(); i!=iEnd; ++i )
    {
        Point::Contour& contour = *i;
        unsigned int uiIndex = 0U;
        for( auto j = contour.begin(), jEnd = contour.end(); j!=jEnd; ++j, ++uiIndex )
        {
            (*j)->uiIndex = uiIndex;
            (*j)->pContour = &contour;
        }
    }

    std::set< Point::Ptr > outstandingPoints;
    for( auto i = areaContours.begin(), iEnd = areaContours.end(); i!=iEnd; ++i )
    {
        const Point::Contour& contour = *i;
        for( auto j = contour.begin(), jEnd = contour.end(); j!=jEnd; ++j )
        {
            outstandingPoints.insert( *j );
        }
    }

    //for the outer contour simply find the point with the highest y value and start from there
    Point::Ptr pIter;
    float fLowestY = boost::numeric::bounds<float>::lowest();
    VERIFY_RTE( -1.0f > fLowestY );
    for( std::set< Point::Ptr >::iterator 
        i = outstandingPoints.begin(),
        iEnd = outstandingPoints.end(); i!=iEnd; ++i )
    {
        if( (*i)->y > fLowestY )
        {
            fLowestY    = (*i)->y;
            pIter       = *i;
        }
    }
    VERIFY_RTE( pIter );


    //create the outer contour;
    blueprintContours.push_back( FloatPairVector() );
    FloatPairVector* pCurrent = &blueprintContours[ 0 ];
    Point::Ptr pFirst = pIter;
    while( !outstandingPoints.empty() )
    {
        VERIFY_RTE( outstandingPoints.find( pIter ) != outstandingPoints.end() );
        outstandingPoints.erase( pIter );

        //first add the point 
        pCurrent->push_back( std::make_pair( pIter->x, pIter->y ) );

        //if the point is a boundary point then determine if we need to switch to
        //other area or have just come from there...
        bool bIncremented = false;
        if( pIter->pBoundary )
        {
            //find the boundary contour
            bool bFound = false;
            for( auto i = boundaryContours.begin(),
                iEnd = boundaryContours.end(); i!=iEnd && !bFound; ++i )
            {
                const Point::Contour& c = *i;
                for( auto j = c.begin(), 
                    jNext = c.begin(),
                        jEnd = c.end(); j!=jEnd; ++j )
                {
                    ++jNext;
                    if( jNext == c.end() ) 
                        jNext = c.begin();

                    if( pIter == *j )
                    {
                        if( pIter->pArea != (*jNext)->pArea )
                        {
                            pIter = *jNext;
                            bIncremented = true;
                        }
                        bFound = true;
                        break;
                    }
                }
            }
            VERIFY_RTE( bFound );
        }
        if( !bIncremented )
        {
            //else simply move to the next point in the polygon
            if( pIter->uiIndex + 1U == pIter->pContour->size() )
                pIter = (*pIter->pContour)[0];
            else
                pIter = (*pIter->pContour)[ pIter->uiIndex + 1U ];
        }

        if( pIter == pFirst )
        {
            //move onto next contour - will be inner contour
            if( !outstandingPoints.empty() )
            {
                pIter = *outstandingPoints.begin();
                pFirst = pIter;
                blueprintContours.push_back( FloatPairVector() );
                pCurrent = &blueprintContours[ blueprintContours.size() - 1 ];
            }
        }
    }
}


}