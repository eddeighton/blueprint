
#include "blueprint/edit.h"

#include "spaces/blueprint.h"
#include "spaces/basicarea.h"

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
const std::string& Connection::TypeName()
{
    static const std::string strTypeName( "connection" );
    return strTypeName;
}

Connection::Connection( Site::Ptr pParent, const std::string& strName )
    :   Site( pParent, strName ),
        m_pSiteParent( pParent )
{

}

Connection::Connection( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName ),
        m_pSiteParent( pParent )
{
}

Node::Ptr Connection::copy( Node::Ptr pParent, const std::string& strName ) const
{   
    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
    VERIFY_RTE( pSiteParent || !pParent );
    return Node::copy< Connection >( 
        boost::dynamic_pointer_cast< const Connection >( shared_from_this() ), pSiteParent, strName );
}

void Connection::init()
{
    Site::init();
    
    if( !( m_pSourceRef = get< Reference >( "source" ) ) )
    {
        m_pSourceRef = Reference::Ptr( new Reference( shared_from_this(), "source" ) );
        m_pSourceRef->init();
        add( m_pSourceRef );
    }
    if( !( m_pTargetRef = get< Reference >( "target" ) ) )
    {
        m_pTargetRef = Reference::Ptr( new Reference( shared_from_this(), "target" ) );
        m_pTargetRef->init();
        add( m_pTargetRef );
    }
    if( !m_pBuffer ) m_pBuffer.reset( new NavBitmap( 1u, 1u ) );
}
void Connection::init( const Ed::Reference& sourceRef, const Ed::Reference& targetRef )
{
    Connection::init();

    m_pSourceRef->setValue( sourceRef );
    m_pTargetRef->setValue( targetRef );
}

void Connection::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );
}

void Connection::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Site::save( node );
}

wykobi::point2d< float > getContourSegmentPointAbsolute( Feature_ContourSegment::Ptr pPoint, Feature_ContourSegment::Points pointType )
{
    ASSERT( pPoint );
    Feature_Contour::Ptr pContour = boost::dynamic_pointer_cast< Feature_Contour >( pPoint->Node::getParent() );
    Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( pContour->Node::getParent() );
    ASSERT( pContour && pArea );
    return wykobi::make_point< float >(
        pArea->getX() + pContour->getX( 0u ) + pPoint->getX( pointType ),
        pArea->getY() + pContour->getY( 0u ) + pPoint->getY( pointType ) );
}

Site::EvaluationResult Connection::evaluate( DataBitmap& data )
{
    EvaluationResult result;

    ASSERT( m_pSourceRef && m_pTargetRef );
    
    RefPtr pSource( shared_from_this(), m_pSourceRef->getValue() );
    RefPtr pTarget( shared_from_this(), m_pTargetRef->getValue() );

    Feature_ContourSegment::Ptr pSourceSegment = pSource.get< Feature_ContourSegment >();
    Feature_ContourSegment::Ptr pTargetSegment = pTarget.get< Feature_ContourSegment >();
    if( pSourceSegment && pTargetSegment )
    {
        std::vector< wykobi::point2d< float > > points, convexHull;
        points.push_back( getContourSegmentPointAbsolute( pSourceSegment, Feature_ContourSegment::eLeft ) );
        points.push_back( getContourSegmentPointAbsolute( pSourceSegment, Feature_ContourSegment::eRight ) );
        points.push_back( getContourSegmentPointAbsolute( pTargetSegment, Feature_ContourSegment::eLeft ) );
        points.push_back( getContourSegmentPointAbsolute( pTargetSegment, Feature_ContourSegment::eRight ) );

        wykobi::algorithm::convex_hull_graham_scan< wykobi::point2d< float > >( 
            points.begin(), points.end(), std::back_inserter( convexHull ) );
    
        wykobi::polygon< float, 2u > contour( convexHull.size() );
        std::copy( convexHull.rbegin(), convexHull.rend(), contour.begin() );

        if( !m_polygonCache || !(m_polygonCache.get().size()==contour.size() ) ||
            !std::equal( contour.begin(), contour.end(), m_polygonCache.get().begin() ) )
        {
            const wykobi::rectangle< float > aabbBox = wykobi::aabb( contour );
            m_ptOffset = sizeBuffer( m_pBuffer, aabbBox );
            Rasteriser ras( m_pBuffer );
            typedef PathImpl::AGGContainerAdaptor< wykobi::polygon< float, 2 > > WykobiPolygonAdaptor;
            typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
            ras.renderPath( Adaptor( WykobiPolygonAdaptor( contour ), true ),
                Rasteriser::ColourType( 255u ), -m_ptOffset.x, -m_ptOffset.y, 0.0f );
            ras.renderPath( Adaptor( WykobiPolygonAdaptor( contour ), true ),
                Rasteriser::ColourType( 1u ), -m_ptOffset.x, -m_ptOffset.y, 1.0f );
            m_pBuffer->setModified();
            m_polygonCache = contour;
        }

        data.makeClaim( DataBitmap::Claim( m_ptOrigin.x + m_ptOffset.x, 
            m_ptOrigin.y + m_ptOffset.y, m_pBuffer, shared_from_this() ) );
    }
        

    return result;
}

void Connection::getContour( FloatPairVector& contourResult ) 
{
    wykobi::point2d< float > ptOrigin = wykobi::make_point( 0.0f, 0.0f );
    Site::Ptr pIter = shared_from_this();
    while( pIter = boost::dynamic_pointer_cast< Site >( pIter->Node::getParent() ) )
        ptOrigin = ptOrigin + wykobi::make_vector( pIter->getX(), pIter->getY() );

    ASSERT( m_pSourceRef && m_pTargetRef );
    
    RefPtr pSource( shared_from_this(), m_pSourceRef->getValue() );
    RefPtr pTarget( shared_from_this(), m_pTargetRef->getValue() );

    Feature_ContourSegment::Ptr pSourceSegment = pSource.get< Feature_ContourSegment >();
    Feature_ContourSegment::Ptr pTargetSegment = pTarget.get< Feature_ContourSegment >();
    if( pSourceSegment && pTargetSegment )
    {
        std::vector< wykobi::point2d< float > > points, convexHullPoints;
        points.push_back( getContourSegmentPointAbsolute( pSourceSegment, Feature_ContourSegment::eLeft ) );
        points.push_back( getContourSegmentPointAbsolute( pSourceSegment, Feature_ContourSegment::eRight ) );
        points.push_back( getContourSegmentPointAbsolute( pTargetSegment, Feature_ContourSegment::eLeft ) );
        points.push_back( getContourSegmentPointAbsolute( pTargetSegment, Feature_ContourSegment::eRight ) );

        wykobi::algorithm::convex_hull_graham_scan< wykobi::point2d< float > >( 
            points.begin(), points.end(), std::back_inserter( convexHullPoints ) );
        
        wykobi::polygon< float, 2u > contour( convexHullPoints.size() );
        std::copy( convexHullPoints.rbegin(), convexHullPoints.rend(), contour.begin() );

        wykobi::polygon< float, 2u > outerContour;
        offsetSimplePolygon( contour, outerContour, 0.1f );
    
        for( wykobi::polygon< float, 2u >::const_iterator 
            i = outerContour.begin(), iEnd = outerContour.end(); i!=iEnd; ++i )
            contourResult.push_back( FloatPair( ptOrigin.x + i->x, ptOrigin.y + i->y ) );
    }
}

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

    std::for_each( dudConnections.begin(), dudConnections.end(),
        boost::bind( &Node::remove, this, _1 ) );
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

Blueprint::EvaluationResult Blueprint::evaluate( DataBitmap& data )
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
        Connection::Ptr pNewConnection( new Connection( shared_from_this(), generateNewNodeName( "connection" ) ) );

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
                pFront->evaluate( data);
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

bool Blueprint::canEditWithTool( const GlyphSpecProducer* pGlyphPrd, unsigned int uiToolType ) const
{
    if( uiToolType == IEditContext::eSelect || 
        uiToolType == IEditContext::eDraw )
        return true;
    else
        return false;
}

void Blueprint::getCmds( CmdInfo::List& cmds ) const
{
}

void Blueprint::getTools( ToolInfo::List& tools ) const
{
}

IInteraction::Ptr Blueprint::beginToolDraw( unsigned int uiTool, float x, float y, float qX, float qY, boost::shared_ptr< Site > pClip )
{
    return IInteraction::Ptr();
}

IInteraction::Ptr Blueprint::beginTool( unsigned int uiTool, float x, float y, float qX, float qY, 
    GlyphSpecProducer* pHit, const std::set< GlyphSpecProducer* >& selection )
{
    return IInteraction::Ptr();
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

    typedef std::map< std::pair< unsigned int, float >, BoundaryPoint > BoundaryPointMap;
}

void Blueprint::getContour( std::vector< FloatPairVector >& blueprintContours )
{
    Point::Contours areaContours;
    Point::Contours boundaryContours;

    unsigned int uiPointIndex = 0u;

    typedef std::map< std::pair< float, float >, Point::Ptr > PointMap;

    PointMap points;

    const Node::PtrVector& areas = getChildren();
    for( Node::PtrVector::const_iterator 
        i = areas.begin(),
        iEnd = areas.end(); i!=iEnd; ++i )
    {
        if( Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( *i ) )
        {
            Point::Contour c;
            Feature_Contour::Ptr pContour = pArea->getContour();
            const wykobi::polygon< float, 2 >& poly = pContour->getPolygon();
            for( auto k = poly.begin(), kEnd = poly.end(); k!=kEnd; ++k )
            {
                Point p{    k->x + pArea->getX(), 
                            k->y + pArea->getY(), 
                            0U, uiPointIndex++, Feature_ContourSegment::Ptr(), pArea, nullptr };
                Point::Ptr pPoint( new Point( p ) );
                c.push_back( pPoint );
                points.insert( std::make_pair( std::make_pair( pPoint->x, pPoint->y ), pPoint ) );
            }


            //add the internal contours
            for( Node::PtrVector::const_iterator 
                j = pArea->getChildren().begin(),
                jEnd = pArea->getChildren().end(); j!=jEnd; ++j )
            {
                if( Area::Ptr pInnerArea = boost::dynamic_pointer_cast< Area >( *j ) )
                {
                    Point::Contour c;
                    Feature_Contour::Ptr pContour = pInnerArea->getContour();
                    const wykobi::polygon< float, 2 >& poly = pContour->getPolygon();
                    for( auto k = poly.begin(), kEnd = poly.end(); k!=kEnd; ++k )
                    {
                        Point p{    k->x + pArea->getX() + pInnerArea->getX(), 
                                    k->y + pArea->getY() + pInnerArea->getY(), 
                                    0U, uiPointIndex++, Feature_ContourSegment::Ptr(), pInnerArea, nullptr };
                        Point::Ptr pPoint( new Point( p ) );
                        c.push_back( pPoint );
                        points.insert( std::make_pair( std::make_pair( pPoint->x, pPoint->y ), pPoint ) );
                    }
                    std::reverse( c.begin(), c.end() );
                    areaContours.push_back( c );
                }
            }

            ///////////////////////////////////////////
            BoundaryPointMap boundaryPoints;
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
                        BoundaryPoint left( pBoundaries ), right( pBoundaries );
                        pBoundaries->getBoundaryPoint( Feature_ContourSegment::eLeft, left.uiIndex, left.x, left.y, left.fRatio );
                        pBoundaries->getBoundaryPoint( Feature_ContourSegment::eRight, right.uiIndex, right.x, right.y, right.fRatio );

                        left.x += pArea->getX() + pContour->getX( 0u );
                        right.x += pArea->getX() + pContour->getX( 0u );

                        left.y += pArea->getY() + pContour->getY( 0u );
                        right.y += pArea->getY() + pContour->getY( 0u );

                        boundaryPoints.insert( std::make_pair( std::make_pair( left.uiIndex, left.fRatio ), left ) );
                        boundaryPoints.insert( std::make_pair( std::make_pair( right.uiIndex, right.fRatio ), right ) );
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