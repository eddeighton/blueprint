

#include "blueprint/connection.h"
#include "blueprint/basicArea.h"
#include "blueprint/spaceUtils.h"

#include "common/assert_verify.hpp"

#include "wykobi_algorithm.hpp"
#include "clipper.hpp"

#include "boost/graph/graph_traits.hpp"
#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/connected_components.hpp"

#include <algorithm>

namespace Blueprint
{

void getSegmentPoints( const Feature_ContourSegment* pFCS, Point2D& pLeft, Point2D& pRight )
{
    Feature_Contour::Ptr pContour = boost::dynamic_pointer_cast< Feature_Contour >(
        pFCS->Node::getParent() );
    Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( 
        pContour->Node::getParent() );
    if( pArea->getTransform().isWindingInverted() )
    {
        pLeft = wykobi::make_point< float >
        (
            pFCS->getX( Feature_ContourSegment::eRight ),
            pFCS->getY( Feature_ContourSegment::eRight )
        );
        pRight = wykobi::make_point< float >
        (
            pFCS->getX( Feature_ContourSegment::eLeft ),
            pFCS->getY( Feature_ContourSegment::eLeft )
        );
    }
    else
    {
        pLeft = wykobi::make_point< float >
        (
            pFCS->getX( Feature_ContourSegment::eLeft ),
            pFCS->getY( Feature_ContourSegment::eLeft )
        );
        pRight = wykobi::make_point< float >
        (
            pFCS->getX( Feature_ContourSegment::eRight ),
            pFCS->getY( Feature_ContourSegment::eRight )
        );
    }
    {
        pLeft.x    += pContour->getX( 0 );
        pLeft.y    += pContour->getY( 0 );
        pRight.x   += pContour->getX( 0 );
        pRight.y   += pContour->getY( 0 );
        {
            pArea->getTransform().transform( pLeft.x,  pLeft.y );
            pArea->getTransform().transform( pRight.x, pRight.y );
        }
    }
}

ConnectionAnalysis::Connection::Connection( const ConnectionAnalysis::ConnectionPair& cp )
{
    m_polygon.reserve( 4 );
    
    {
        Point2D pLeft, pRight;
        getSegmentPoints( cp.first, pLeft, pRight );
        
        m_polygon.push_back( pLeft );
        m_polygon.push_back( pRight );
    }
        
    {
        Point2D pLeft, pRight;
        getSegmentPoints( cp.second, pLeft, pRight );
        
        m_polygon.push_back( pLeft );
        m_polygon.push_back( pRight );
    }
    
}
        
void ConnectionAnalysis::calculate()
{
    m_connections.clear();
    m_fcs.clear();
    m_fcsPairs.clear();
    m_areaIDMap.clear();
    m_areaIDTable.clear();
    m_totalComponents = 0;
    m_components.clear();
    
    calculateConnections();
    calculateConnectedComponents();
}
        
void ConnectionAnalysis::calculateConnections()
{
    struct CompareFCSID
    {
        inline bool operator()( const FCSID& left, const FCSID& right ) const
        {
            return ( left.angle     != right.angle )    ? left.angle    < right.angle :
                   ( left.x         != right.x )        ? left.x        < right.x :
                   ( left.y         != right.y )        ? left.y        < right.y :
                   false;
        }
    };
    
    using BroadPhaseLookup = std::multimap< FCSID, const FCSValue*, CompareFCSID >;
    
    BroadPhaseLookup lookup;
    
    for( Site::PtrVector::const_iterator 
        i = m_area.getSpaces().begin(),
        iEnd = m_area.getSpaces().end(); i!=iEnd; ++i )
    {
        if( Area::Ptr pChildArea = boost::dynamic_pointer_cast< Area >( *i ) )
        {
            Feature_Contour::Ptr pContour = pChildArea->getContour();
            
            for( Feature_ContourSegment::Ptr pContourSegment : pChildArea->getBoundaries() )
            {
                Point2D ptContourBase;
                {
                    Feature_Contour::Ptr pContour = 
                        boost::dynamic_pointer_cast< Feature_Contour >( pContourSegment->Node::getParent() );
                    ptContourBase.x = pContour->getX( 0 );
                    ptContourBase.y = pContour->getY( 0 );
                }
                
                Point2D ptMid;
                {
                    ptMid.x = ptContourBase.x + pContourSegment->getX( Feature_ContourSegment::eMidPoint );
                    ptMid.y = ptContourBase.y + pContourSegment->getY( Feature_ContourSegment::eMidPoint );
                    pChildArea->getTransform().transform( ptMid.x, ptMid.y );
                }
                
                Math::Angle< 8 >::Value angle;
                {
                    Point2D ptEdge;
                    {
                        if( pChildArea->getTransform().isWindingInverted() )
                        {
                            ptEdge.x = ptContourBase.x + pContourSegment->getX( Feature_ContourSegment::eLeft );
                            ptEdge.y = ptContourBase.y + pContourSegment->getY( Feature_ContourSegment::eLeft );
                        }
                        else
                        {
                            ptEdge.x = ptContourBase.x + pContourSegment->getX( Feature_ContourSegment::eRight );
                            ptEdge.y = ptContourBase.y + pContourSegment->getY( Feature_ContourSegment::eRight );
                        }
                        pChildArea->getTransform().transform( ptEdge.x, ptEdge.y );
                    }
                    angle = Math::fromVector< Math::Angle< 8 > >( ptEdge.x - ptMid.x, ptEdge.y - ptMid.y );
                }
                    
                const FCSID id
                { 
                    static_cast< Math::Angle< 8 >::Value >( ( angle + 2 ) % 8 ),
                    static_cast< int >( Math::quantize( ptMid.x, fQuantisation ) ),
                    static_cast< int >( Math::quantize( ptMid.y, fQuantisation ) ) 
                };
                
                const FCSValue value{ id, pContourSegment.get(), ptMid.x, ptMid.y };
                
                m_fcs.push_back( value );
            }
        }
    }
    
    for( const FCSValue& value : m_fcs )
    {
        lookup.insert( std::make_pair( value.id, &value ) );
    }
    
    using FCSValuePtrSet = std::set< const FCSValue* >;
    FCSValuePtrSet openList;
    for( const FCSValue& value : m_fcs )
    {
        openList.insert( &value );
    }
    
    for( const FCSValue& value : m_fcs )
    {
        const FCSValue* pValue = &value;
        if( openList.find( pValue ) != openList.end() )
        {
            bool bFound = false;
            FCSID id = pValue->id;
            id.angle = Math::opposite< Math::Angle< 8 > >( id.angle );
            for( id.x = pValue->id.x - fQuantisation; ( id.x != pValue->id.x + fQuantisation ) && !bFound; id.x += fQuantisation )
            {
                for( id.y = pValue->id.y - fQuantisation; ( id.y != pValue->id.y + fQuantisation ) && !bFound; id.y += fQuantisation )
                {
                    BroadPhaseLookup::iterator i        = lookup.lower_bound( id );
                    BroadPhaseLookup::iterator iUpper   = lookup.upper_bound( id );
                    for( ; i!=iUpper && !bFound; ++i )
                    {
                        const FCSValue* pCompare = i->second;
                        if( openList.find( pCompare ) != openList.end() )
                        {
                            if( wykobi::distance( pValue->x, pValue->y, pCompare->x, pCompare->y ) < fConnectionMaxDist )
                            {
                                ConnectionPair cp{ pValue->pFCS, pCompare->pFCS };
                                
                                Connection::Ptr pConnection( new Connection( cp ) );
                                
                                auto r = m_connections.insert( std::make_pair( cp, pConnection ) );
                                VERIFY_RTE( r.second );
                                
                                m_fcsPairs.push_back( std::make_pair( pValue, pCompare ) );
                                
                                openList.erase( pValue );
                                openList.erase( pCompare );
                                bFound = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

void ConnectionAnalysis::calculateConnectedComponents()
{
    using Graph = boost::adjacency_list< 
        boost::vecS, boost::vecS, boost::undirectedS >;
    
    for( Site::Ptr pSite : m_area.getSpaces() )
    {
        if( Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( pSite ) )
        {
            m_areaIDMap[ pArea.get() ] = m_areaIDTable.size();
            m_areaIDTable.push_back( pArea.get() );
        }
    }

    Graph graph( m_areaIDTable.size() );
    
    for( const FCSValue::CstPtrPair& cp : m_fcsPairs )
    {
        int idOne, idTwo;
        {
            const Feature_ContourSegment* pFCS = cp.first->pFCS;
            Feature_Contour::Ptr pContour = 
                boost::dynamic_pointer_cast< Feature_Contour >( pFCS->Node::getParent() );
            Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( 
                pContour->Node::getParent() );
            idOne = m_areaIDMap[ pArea.get() ];
        }
        {        
            const Feature_ContourSegment* pFCS = cp.second->pFCS;
            Feature_Contour::Ptr pContour = 
                boost::dynamic_pointer_cast< Feature_Contour >( pFCS->Node::getParent() );
            Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( 
                pContour->Node::getParent() );
            idTwo = m_areaIDMap[ pArea.get() ];
        }
        
        ASSERT( idOne != idTwo );
        boost::add_edge( idOne, idTwo, graph );
    }
    
    if( !m_areaIDTable.empty() )
    {
        m_components.resize( m_areaIDTable.size() );
        m_totalComponents =
            boost::connected_components( graph, &m_components[ 0 ] );
    }
    else
    {
        m_totalComponents = 0;
    }
}

bool ConnectionAnalysis::isFeatureContourSegmentConnected( Feature_ContourSegment* pFeatureContourSegment ) const
{
    for( ConnectionPairMap::const_iterator i = m_connections.begin(),
        iEnd = m_connections.end(); i!=iEnd; ++i )
    {
        if( i->first.first == pFeatureContourSegment || 
            i->first.second == pFeatureContourSegment  )
        return true;    
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
void ExteriorAnalysis::calculate()
{
    static const double CLIPPER_MAG = 100.0;
            
    m_exteriors.clear();
    
    const int iTotalComponents                              = m_connections.getTotalComponents();
    const ConnectionAnalysis::AreaIDTable& areaTable        = m_connections.getAreaIDTable();
    const ConnectionAnalysis::ComponentVector& components   = m_connections.getComponents();
    
    ClipperLib::Path areaInteriorPath;
    {
        const Polygon2D& polygon = m_area.getContour()->getPolygon();
        for( const Point2D& pt : polygon )
        {
            areaInteriorPath.push_back( 
                ClipperLib::IntPoint( 
                    static_cast< ClipperLib::cInt >( pt.x * CLIPPER_MAG ), 
                    static_cast< ClipperLib::cInt >( pt.y * CLIPPER_MAG ) ) ); 
        }
        if( wykobi::polygon_orientation( polygon ) == wykobi::Clockwise )
            std::reverse( areaInteriorPath.begin(), areaInteriorPath.end() );
    }
    
    for( int i = 0; i < iTotalComponents; ++i )
    {
        std::set< const Area* > areas;
        for( int j = 0; j != areaTable.size(); ++j )
        {
            if( components[ j ] == i )
            {
                const Area* pArea = areaTable[ j ];
                areas.insert( pArea );
            }
        }
        
        if( !areas.empty() )
        {
            //create exterior for the area set...
            ClipperLib::Paths allPaths;
            {
                for( const Area* pArea : areas )
                {
                    ClipperLib::Path inputClipperPath;
                
                    //get the orientated transformed contour
                    Polygon2D polygon = pArea->getContour()->getPolygon();
                    for( Point2D& pt : polygon )
                    {
                        pArea->getTransform().transform( pt.x, pt.y );
                        inputClipperPath.push_back( 
                            ClipperLib::IntPoint( 
                                static_cast< ClipperLib::cInt >( pt.x * CLIPPER_MAG ), 
                                static_cast< ClipperLib::cInt >( pt.y * CLIPPER_MAG ) ) ); 
                    }
                    if( wykobi::polygon_orientation( polygon ) == wykobi::Clockwise )
                        std::reverse( inputClipperPath.begin(), inputClipperPath.end() );
                    
                    ClipperLib::Paths extrudedPaths;
                    {
                        ClipperLib::ClipperOffset co;
                        co.AddPath( inputClipperPath, ClipperLib::jtSquare, ClipperLib::etClosedPolygon );
                        co.Execute( extrudedPaths, fExtrusionAmt * CLIPPER_MAG );
                    }
                    std::copy( extrudedPaths.begin(), extrudedPaths.end(),
                        std::back_inserter( allPaths ) );
    
                }
            }
            
            if( !allPaths.empty() )
            {
                ClipperLib::Clipper unionClipper;
                if( unionClipper.AddPaths( allPaths, ClipperLib::ptClip, true ) )
                {
                    ClipperLib::Paths unionPaths;
                    if( unionClipper.Execute( ClipperLib::ctUnion, unionPaths, 
                        ClipperLib::pftPositive, ClipperLib::pftPositive ) )
                    {
                        ClipperLib::Paths clippedUnionPaths;
                        {
                            ClipperLib::Clipper interiorClip;
                            if( interiorClip.AddPaths( unionPaths, ClipperLib::ptSubject, true ) )
                            {
                                if( interiorClip.AddPath( areaInteriorPath, ClipperLib::ptClip, true ) )
                                {
                                    if( interiorClip.Execute( ClipperLib::ctIntersection, clippedUnionPaths, 
                                        ClipperLib::pftPositive, ClipperLib::pftPositive ) )
                                    {
                                        for( const ClipperLib::Path& unionPath : clippedUnionPaths )
                                        {
                                            Exterior::Ptr pExterior( new Exterior );
                                            m_exteriors.push_back( pExterior );
                                            pExterior->m_areas.assign( areas.begin(), areas.end() );
                                                    
                                            for( ClipperLib::Path::const_iterator 
                                                j = unionPath.begin(), jEnd = unionPath.end(); j!=jEnd; ++j )
                                            {
                                                pExterior->m_polygon.push_back( 
                                                    wykobi::make_point< float >( static_cast<float>(j->X / CLIPPER_MAG),
                                                                                 static_cast<float>(j->Y / CLIPPER_MAG) ) );
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
        
       

}