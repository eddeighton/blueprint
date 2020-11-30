
#include "blueprint/compiler.h"
#include "blueprint/connection.h"
#include "blueprint/cgalSettings.h"

#include "common/angle.hpp"
#include "common/file.hpp"

#include "blueprint/cgalSettings.h"

#include <iostream>

namespace 
{
    
static const std::vector< const char* > SVG_COLOURS =
{
    "blue",
    "green",
    "red",
    "yellow",
    "orange",
    "purple",
    "brown",
    "black"                   
};

inline double to_double( const CGAL::Quotient< CGAL::MP_Float >& q )
{
    return 
        CGAL::INTERN_MP_FLOAT::to_double( q.numerator() ) /
        CGAL::INTERN_MP_FLOAT::to_double( q.denominator() );
}

void svgLine( Blueprint::Arr_with_hist_2::Halfedge_const_handle h, double minX, double minY, double scale, const char* pszColour, std::ostream& os )
{
    if( h->target()->point() == h->curve().source() )
        h = h->twin();
    
    const double startX = ( to_double(  h->source()->point().x() ) - minX ) * scale;
    const double startY = ( to_double( -h->source()->point().y() ) - minY ) * scale;
    const double endX   = ( to_double(  h->target()->point().x() ) - minX ) * scale;
    const double endY   = ( to_double( -h->target()->point().y() ) - minY ) * scale;
    
    std::ostringstream osEdge;
    osEdge << 
        startX << "," << startY << " " << 
        ( startX + ( endX - startX ) / 2.0 ) << "," << ( startY + ( endY - startY ) / 2.0 ) << " " << 
        endX << "," << endY;
    
    os << "       <polyline points=\"" << osEdge.str() << "\" style=\"fill:none;stroke:" << pszColour << ";stroke-width:1\" marker-mid=\"url(#mid)\" />\n";
    os << "       <circle cx=\"" << startX << "\" cy=\"" << startY << "\" r=\"3\" stroke=\"" << pszColour << "\" stroke-width=\"1\" fill=\"" << pszColour << "\" />\n";
    os << "       <circle cx=\"" << endX << "\" cy=\"" << endY << "\" r=\"3\" stroke=\"" << pszColour << "\" stroke-width=\"1\" fill=\"" << pszColour << "\" />\n";

}

                

}

namespace
{
    class ContourPoint;
    
    struct Boundary
    {
        unsigned int edge;
        wykobi::point2d< float > start, end;
        float distance;
        const Blueprint::Feature_ContourSegment* pFCS;
    };
    
    struct ExteriorBoundary
    {
        unsigned int edge;
        wykobi::point2d< float > start, end;
        float distance;
        const Blueprint::Feature_ContourSegment* pFCS;
        std::shared_ptr< ContourPoint >
            m_pFirstStart, m_pFirstEnd, 
            m_pSecondStart, m_pSecondEnd;
    };
    
    struct ConnectionCurves
    {
        using Ptr           = std::shared_ptr< ConnectionCurves >;
        using PtrVector     = std::vector< Ptr >;
        
        std::shared_ptr< ContourPoint >
            m_pFirstStart, m_pFirstEnd, m_pFirstMid, 
            m_pSecondStart, m_pSecondEnd, m_pSecondMid;
            
            
        Blueprint::Arr_with_hist_2::Vertex_handle m_vFirstStart,  m_vFirstEnd;
        Blueprint::Arr_with_hist_2::Vertex_handle m_vSecondStart, m_vSecondEnd;
        Blueprint::Arr_with_hist_2::Halfedge_handle m_hDoorStep;
        
        void create( Blueprint::Arr_with_hist_2& arr );
        void finish( Blueprint::Arr_with_hist_2& arr );
    };
    
    struct ContourPoint
    {
        using Ptr           = std::shared_ptr< ContourPoint >;
        using PtrVector     = std::vector< Ptr >;
        
        using ContourPointPtrPair = std::pair< Ptr, Ptr >;
        using ContourPointPtrPairVector = std::vector< ContourPointPtrPair >;
        
        using Angle8Traits  = Math::Angle< 8 >;
        using Angle8        = Angle8Traits::Value;
        using Point2D       = wykobi::point2d< float >;
        using Segment2D     = wykobi::segment< float, 2 >;
        using Vector2D      = wykobi::vector2d< float >;
        
        friend void getInteriorContourPoints( const Blueprint::Matrix& transform,
                const Blueprint::ConnectionAnalysis* pConnections, 
                const Blueprint::Area* pArea, 
                const std::vector< Boundary >& boundaries,
                const wykobi::polygon< float, 2 >& contour,
                ContourPoint::PtrVector& points, 
                ContourPoint::ContourPointPtrPairVector& interiorConnectionPairs,
                ContourPoint::ContourPointPtrPairVector& exteriorConnectionPairs );
                        
        enum PointType
        {
            Normal,
            Start,
            End
        };
        
        ContourPoint( const Blueprint::Area* pArea, bool bInterior, PointType _type, Point2D _pt,
                    const Blueprint::Feature_ContourSegment* _pFCS )
            :   m_pArea( pArea ),
                m_bInterior( bInterior ),
                m_type( _type ),
                m_pt( _pt ),
                m_pFCS( _pFCS )
        {
        }
        
        const Blueprint::Area* getArea() const { return m_pArea; }
        bool isInterior() const { return m_bInterior; }
        
        PointType getType() const { return m_type; }
        const Point2D& getPoint() const { return m_pt; }
        const Blueprint::Feature_ContourSegment* getFCS() const { return m_pFCS; }
        
        Blueprint::Curve_handle getCurveHandle() const { return m_ch; }
        void setCurveHandle( Blueprint::Curve_handle ch ) { m_ch = ch; }
        
    private:
        const Blueprint::Area* m_pArea;
        bool m_bInterior;
        PointType m_type;
        Point2D m_pt;
        const Blueprint::Feature_ContourSegment* m_pFCS = nullptr;
        Blueprint::Curve_handle m_ch;
    };
    
    void ConnectionCurves::create( Blueprint::Arr_with_hist_2& arr )
    {
        VERIFY_RTE( m_pFirstStart->getArea() ==  m_pFirstEnd->getArea() );
        VERIFY_RTE( m_pSecondStart->getArea() == m_pSecondEnd->getArea() );
        VERIFY_RTE( m_pFirstStart->getArea() !=  m_pSecondStart->getArea() );
        
        const Blueprint::Point_2 ptFirstStart( m_pFirstStart->getPoint().x, m_pFirstStart->getPoint().y );
        const Blueprint::Point_2 ptFirstEnd(   m_pFirstEnd->getPoint().x,   m_pFirstEnd->getPoint().y ); 
        Blueprint::Curve_handle hFirstDoorStep = 
            CGAL::insert( arr, Blueprint::Segment_2( ptFirstStart, ptFirstEnd ) );
                
        const Blueprint::Point_2 ptSecondStart( m_pSecondStart->getPoint().x, m_pSecondStart->getPoint().y );
        const Blueprint::Point_2 ptSecondEnd(   m_pSecondEnd->getPoint().x,   m_pSecondEnd->getPoint().y ); 
        Blueprint::Curve_handle hSecondDoorStep = 
            CGAL::insert( arr, Blueprint::Segment_2( ptSecondStart, ptSecondEnd ) );
            
        const Blueprint::Point_2 ptFirstMid(  m_pFirstMid->getPoint().x,  m_pFirstMid->getPoint().y );
        const Blueprint::Point_2 ptSecondMid( m_pSecondMid->getPoint().x, m_pSecondMid->getPoint().y ); 
                
        {
            int iFoundFirstStart = 0,  iFoundFirstEnd = 0;
            int iFoundSecondStart = 0, iFoundSecondEnd = 0;
            for( Blueprint::Arr_with_hist_2::Vertex_iterator 
                    i = arr.vertices_begin(),
                    iEnd = arr.vertices_end(); i!=iEnd; ++i )
            {
                if( i->point() == ptFirstStart )
                {
                    ++iFoundFirstStart;
                    m_vFirstStart = i;
                }
                if( i->point() == ptFirstEnd )
                {
                    ++iFoundFirstEnd;
                    m_vFirstEnd = i;
                }
                if( i->point() == ptSecondStart )
                {
                    ++iFoundSecondStart;
                    m_vSecondStart = i;
                }
                if( i->point() == ptSecondEnd )
                {
                    ++iFoundSecondEnd;
                    m_vSecondEnd = i;
                }
            }
            VERIFY_RTE( ( iFoundFirstStart == 1 )   && ( iFoundFirstEnd == 1 ) );
            VERIFY_RTE( ( iFoundSecondStart == 1 )  && ( iFoundSecondEnd == 1 ) );
            VERIFY_RTE( ( iFoundFirstStart == 1 )   && ( iFoundFirstEnd == 1 ) );
            VERIFY_RTE( ( iFoundSecondStart == 1 )  && ( iFoundSecondEnd == 1 ) );
        }
        
        //create edge to mid points
        const Blueprint::Segment_2 firstStartToMid(  ptFirstStart,  ptFirstMid );
        const Blueprint::Segment_2 firstEndToMid(    ptFirstEnd,    ptSecondMid );
        const Blueprint::Segment_2 secondStartToMid( ptSecondStart, ptSecondMid );
        const Blueprint::Segment_2 secondEndToMid(   ptSecondEnd,   ptFirstMid );
        
        Blueprint::Arr_with_hist_2::Vertex_handle vFirstMid;
        {
            if( ptFirstStart < ptFirstMid )
            {
                Blueprint::Arr_with_hist_2::Halfedge_handle hFirstStartToMid = 
                    arr.insert_from_left_vertex( firstStartToMid, m_vFirstStart );
                vFirstMid = hFirstStartToMid->target();
            }
            else
            {
                Blueprint::Arr_with_hist_2::Halfedge_handle hFirstStartToMid = 
                    arr.insert_from_right_vertex( firstStartToMid, m_vFirstStart );
                vFirstMid = hFirstStartToMid->target();
            }
        }
        
        Blueprint::Arr_with_hist_2::Vertex_handle vSecondMid;
        {
            if( ptFirstEnd < ptSecondMid )
            {
                Blueprint::Arr_with_hist_2::Halfedge_handle hFirstEndToMid = 
                    arr.insert_from_left_vertex( firstEndToMid, m_vFirstEnd );
                vSecondMid = hFirstEndToMid->target();
            }
            else
            {
                Blueprint::Arr_with_hist_2::Halfedge_handle hFirstEndToMid = 
                    arr.insert_from_right_vertex( firstEndToMid, m_vFirstEnd );
                vSecondMid = hFirstEndToMid->target();
            }
        }
        
        {
            Blueprint::Arr_with_hist_2::Halfedge_handle hSecondStartToMid = 
                arr.insert_at_vertices( secondStartToMid, m_vSecondStart, vSecondMid );
            Blueprint::Arr_with_hist_2::Halfedge_handle hSecondEndToMid   = 
                arr.insert_at_vertices( secondEndToMid, m_vSecondEnd, vFirstMid );
        }
        
        //create edge between mid points
        {
            const Blueprint::Segment_2 segDoorStep( ptFirstMid, ptSecondMid );
            m_hDoorStep = arr.insert_at_vertices( segDoorStep, vFirstMid, vSecondMid );
        }
        
    }
    void ConnectionCurves::finish( Blueprint::Arr_with_hist_2& arr )
    {
        m_hDoorStep->set_data( m_pFirstStart->getArea() );
        m_hDoorStep->twin()->set_data( m_pSecondStart->getArea() );
        
        //remove edge between m_vFirstStart m_vFirstEnd
        {
            bool bFound = false;
            Blueprint::Arr_with_hist_2::Halfedge_around_vertex_circulator first, iter;
            first = iter = m_vFirstStart->incident_halfedges();
            do
            {
                if( iter->source() == m_vFirstEnd )
                {
                    arr.remove_edge( iter );
                    bFound = true;
                    break;
                }
                ++iter;
            }
            while( iter != first );
            VERIFY_RTE( bFound );
        }

        //remove edge between m_vSecondStart m_vSecondEnd
        {
            bool bFound = false;
            Blueprint::Arr_with_hist_2::Halfedge_around_vertex_circulator first, iter;
            first = iter = m_vSecondStart->incident_halfedges();
            do
            {
                if( iter->source() == m_vSecondEnd )
                {
                    arr.remove_edge( iter );
                    bFound = true;
                    break;
                }
                ++iter;
            }
            while( iter != first );
            VERIFY_RTE( bFound );
        }
    }
    
    void getInteriorBoundaries( const Blueprint::ConnectionAnalysis& connectionAnalysis, 
                        const Blueprint::Area* pArea,
                        std::vector< Boundary >& boundaries )
    {
        for( Blueprint::Feature_ContourSegment::Ptr pBoundaryPtr : pArea->getBoundaries() )
        {
            if( connectionAnalysis.isFeatureContourSegmentConnected( pBoundaryPtr.get() ) || 
                pBoundaryPtr->isSegmentExterior() )
            {
                unsigned int polyIndex = 0; 
                {
                    float fDistance = 0.0f;
                    ContourPoint::Point2D midPoint;
                    pBoundaryPtr->getBoundaryPoint( 
                        Blueprint::Feature_ContourSegment::eMidPoint, 
                        polyIndex, midPoint.x, midPoint.y, fDistance );
                }
                    
                float startfDistance = 0.0f;
                unsigned int startpolyIndex = 0; 
                ContourPoint::Point2D start;
                {
                    pBoundaryPtr->getBoundaryPoint( 
                        Blueprint::Feature_ContourSegment::eLeft, 
                        startpolyIndex, start.x, start.y, startfDistance );
                }
                    
                unsigned int endpolyIndex = 0; 
                ContourPoint::Point2D end;
                {
                    float endfDistance = 0.0f;
                    pBoundaryPtr->getBoundaryPoint( 
                        Blueprint::Feature_ContourSegment::eRight, 
                        endpolyIndex, end.x, end.y, endfDistance );
                }
                    
                const ContourPoint::Point2D offset( 
                    wykobi::make_point< float >( 
                        pArea->getContour()->getX( 0u ), 
                        pArea->getContour()->getY( 0u ) ) );
                    
                VERIFY_RTE_MSG( ( polyIndex == startpolyIndex ) && ( polyIndex == endpolyIndex ), 
                    "Boundary overlaps segment edges" );
                    
                boundaries.push_back( 
                    Boundary
                    { 
                        polyIndex, start + offset, end + offset, startfDistance , pBoundaryPtr.get()
                    } );
            }
        }
    }
    
    void getInteriorContourPoints( const Blueprint::Matrix& transform,
            const Blueprint::ConnectionAnalysis* pConnections, 
            const Blueprint::Area* pArea, 
            const std::vector< Boundary >& boundaries,
            const wykobi::polygon< float, 2 >& contour,
            ContourPoint::PtrVector& points, 
            ContourPoint::ContourPointPtrPairVector& interiorConnectionPairs,
            ContourPoint::ContourPointPtrPairVector& exteriorConnectionPairs )
    {
        ContourPoint::ContourPointPtrPairVector newInteriorConnections;
        ContourPoint::ContourPointPtrPairVector newExteriorConnections;
        {
            int totalBoundaries = 0;
            unsigned int iEdgeCounter = 0;
            for( wykobi::polygon< float, 2 >::const_iterator 
                i = contour.begin(),
                iEnd = contour.end(); i!=iEnd; ++i, ++iEdgeCounter )
            {
                points.push_back( ContourPoint::Ptr( 
                    new ContourPoint( pArea, true, ContourPoint::Normal, *i, nullptr ) ) );
                
                std::map< float, const Boundary* > inSegmentBoundaries;
                for( const Boundary& boundary : boundaries )
                {
                    if( boundary.edge == iEdgeCounter )
                    {
                        auto result = 
                            inSegmentBoundaries.insert( std::make_pair( boundary.distance, &boundary ) );
                        VERIFY_RTE_MSG( result.second, "Duplicate segment distance error" );
                    }
                }
                
                for( const auto& boundPair : inSegmentBoundaries )
                {
                    const std::size_t sz = points.size();
                    points.push_back( ContourPoint::Ptr( 
                        new ContourPoint( pArea, true, ContourPoint::Start,  boundPair.second->start, boundPair.second->pFCS ) ) );
                    points.push_back( ContourPoint::Ptr( 
                        new ContourPoint( pArea, true, ContourPoint::End,    boundPair.second->end  , boundPair.second->pFCS ) ) );
                    ++totalBoundaries;
                    
                    if( boundPair.second->pFCS->isSegmentExterior() )
                    {
                        newExteriorConnections.push_back(
                            std::make_pair( points[ sz + 1U ], points[ sz ] ) );
                    }
                    else
                    {
                        newInteriorConnections.push_back(
                            std::make_pair( points[ sz ], points[ sz + 1U ] ) );
                    }
                }
            }
            VERIFY_RTE_MSG( totalBoundaries == boundaries.size(), "Boundary error" );
        }
        
        //calculate absolute points
        {
            for( ContourPoint::Ptr pPoint : points )
            {
                transform.transform( pPoint->m_pt.x, pPoint->m_pt.y );
            }
        }
        
        //check final orientation
        bool bFlipped = false;
        {
            wykobi::polygon< float, 2 > completeContour;
            for( ContourPoint::Ptr pPoint : points )
            {
                completeContour.push_back( pPoint->m_pt );
            }
            const int polyOrientation = wykobi::polygon_orientation( completeContour );
            if( polyOrientation != wykobi::CounterClockwise )
            {
                bFlipped = true;
                std::reverse( points.begin(), points.end() );
            }
        }
        
        //capture connection pairs
        {
            for( auto& p : newExteriorConnections )
            {
                if( bFlipped )
                {
                    auto t = p;
                    p.first = t.second;
                    p.second = t.first;
                }
                exteriorConnectionPairs.push_back( p );
            }
            for( auto& p : newInteriorConnections )
            {
                if( bFlipped )
                {
                    auto t = p;
                    p.first = t.second;
                    p.second = t.first;
                }
                interiorConnectionPairs.push_back( p );
            }
        }
        
    }
    
    void getExteriorBoundaries( 
        const wykobi::polygon< float, 2u >& exteriorPolygon,
        const ContourPoint::ContourPointPtrPairVector& exteriorConnectionPairs,
        std::vector< ExteriorBoundary >& boundaries )
    {
        unsigned int szEdgeIndex = 0;
        for( wykobi::polygon< float, 2u >::const_iterator
            i     = exteriorPolygon.begin(),
            iNext = exteriorPolygon.begin(),
            iEnd  = exteriorPolygon.end();
            i != iEnd;
            ++i, ++szEdgeIndex )
        {
            ++iNext;
            if( iNext == iEnd ) 
                iNext = exteriorPolygon.begin();
            
            const Math::Angle< 8 >::Value exteriorAngle = 
                Math::fromVector< Math::Angle< 8 > >( iNext->x - i->x, iNext->y - i->y );
                
            const ContourPoint::Segment2D exteriorLine = wykobi::make_segment( *i, *iNext );
            
            for( const ContourPoint::ContourPointPtrPair& ecp : exteriorConnectionPairs )
            {
                ContourPoint::Ptr pStart  = ecp.first;
                ContourPoint::Ptr pEnd    = ecp.second;
                
                //determine if exterior line segment is valid for interior connection
                const Math::Angle< 8 >::Value interiorAngle = 
                    Math::fromVector< Math::Angle< 8 > >( 
                        pEnd->getPoint().x - pStart->getPoint().x, 
                        pEnd->getPoint().y - pStart->getPoint().y );
                        
                        
                //outer contour is clockwise - interior is counter-clockwise - so these two angles
                //just need to be equal for them to be in opposing direction.
                if( interiorAngle == exteriorAngle )
                {
                    const ContourPoint::Point2D ptStart =
                        closest_point_on_segment_from_point( exteriorLine, pStart->getPoint() );
                    const float fStartDist = wykobi::distance( pStart->getPoint(), ptStart );
                    
                    const ContourPoint::Point2D ptEnd =
                        closest_point_on_segment_from_point( exteriorLine, pEnd->getPoint() );
                    const float fEndDist = wykobi::distance( pEnd->getPoint(), ptEnd );
                        
                    if( ( fStartDist >= Blueprint::ConnectionAnalysis::fConnectionMinDist ) &&
                        ( fStartDist <= Blueprint::ConnectionAnalysis::fConnectionMaxDist ) &&
                        ( fEndDist >= Blueprint::ConnectionAnalysis::fConnectionMinDist   ) &&
                        ( fEndDist <= Blueprint::ConnectionAnalysis::fConnectionMaxDist   ) )
                    {
                        const float startfDistance = wykobi::distance( *i, ptStart );
                            
                        std::cout << "Exterior boundary " << 
                            " area: " << pStart->getFCS()->Node::getParent()->getParent()->getName() << 
                            " boundary: " << pStart->getFCS()->Node::getName() <<
                            "(" << ptStart.x << "," << ptStart.y << ") (" << ptEnd.x << "," << ptEnd.y << ")" << 
                            " startfDistance: "     << startfDistance << 
                            " fStartDist: "         << fStartDist << 
                            " fEndDist: "           << fEndDist << 
                            std::endl;
                            
                        boundaries.push_back(
                            ExteriorBoundary
                            { 
                                szEdgeIndex, ptStart, ptEnd, startfDistance, pStart->getFCS(), pStart, pEnd
                            } );
                    }
                }
            }
        }
    }
    
    void getExteriorContourPoints( 
            const Blueprint::ConnectionAnalysis& connectionAnalysis, 
            const Blueprint::Area* pArea,
            const wykobi::polygon< float, 2 >& contour, 
            std::vector< ExteriorBoundary >& boundaries,
            ContourPoint::PtrVector& points)
    {
        {
            int totalBoundaries = 0;
            unsigned int iEdgeCounter = 0;
            for( wykobi::polygon< float, 2 >::const_iterator 
                i = contour.begin(),
                iEnd = contour.end(); i!=iEnd; ++i, ++iEdgeCounter )
            {
                points.push_back( ContourPoint::Ptr( 
                    new ContourPoint( pArea, false, ContourPoint::Normal, *i, nullptr ) ) );
                
                std::map< float, ExteriorBoundary* > inSegmentBoundaries;
                for( ExteriorBoundary& boundary : boundaries )
                {
                    if( boundary.edge == iEdgeCounter )
                    {
                        auto result = 
                            inSegmentBoundaries.insert( std::make_pair( boundary.distance, &boundary ) );
                        VERIFY_RTE_MSG( result.second, "Exterior boundary duplicate segment distance error: " << 
                                " exterior area: " << pArea->Node::getName() << 
                                " interior area: " << boundary.pFCS->Node::getParent()->getParent()->getName() << 
                                " boundary: " << boundary.pFCS->Node::getName() );
                    }
                }
                
                for( auto& boundPair : inSegmentBoundaries )
                {
                    const std::size_t sz = points.size();
                    points.push_back( ContourPoint::Ptr( 
                        new ContourPoint( pArea, false, ContourPoint::Start,  boundPair.second->start, boundPair.second->pFCS ) ) );
                    points.push_back( ContourPoint::Ptr( 
                        new ContourPoint( pArea, false, ContourPoint::End,    boundPair.second->end  , boundPair.second->pFCS ) ) );
                        
                    {
                        //reverse order of the points
                        boundPair.second->m_pSecondStart = points[ sz + 1U ];
                        boundPair.second->m_pSecondEnd = points[ sz ];
                    }
                    ++totalBoundaries;
                }
            }
            VERIFY_RTE_MSG( totalBoundaries == boundaries.size(), "Boundary error" );
        }
    }
    
    bool getAbsoluteOrientedContour( const Blueprint::Matrix& transform, int iOrientation, wykobi::polygon< float, 2u >& contour )
    {
        for( ContourPoint::Point2D& pt : contour )
        {
            transform.transform( pt.x, pt.y );
        }
        const int polyOrientation = wykobi::polygon_orientation( contour );
        if( polyOrientation != iOrientation )
        {
            std::reverse( contour.begin(), contour.end() );
            return true;
        }
        else
        {
            return false;
        }
    }
}

namespace Blueprint
{
    
class Compiler::CompilerImpl
{
    struct ExteriorInfo
    {
        using Ptr = std::shared_ptr< ExteriorInfo >;
        using PtrMap = std::map< const ExteriorAnalysis::Exterior*, Ptr >;
        using PtrVector = std::vector< Ptr >;
        
        const Area* pArea;
        ExteriorAnalysis::Exterior* pExterior;
        std::vector< ExteriorBoundary > boundaries;
        ContourPoint::PtrVector exteriorPoints;
        ConnectionCurves::PtrVector connectionCurves;
    };
    ExteriorInfo::PtrMap m_exteriorInfoMap;
    
    struct AreaInfo
    {
        using Ptr = std::shared_ptr< AreaInfo >;
        using PtrMap = std::map< const Area*, Ptr >;
        using PtrVector = std::vector< Ptr >;
        
        const Area* pArea;
        std::vector< Boundary > boundaries;
        ContourPoint::PtrVector interiorPoints;
        ConnectionCurves::PtrVector connectionCurves;
        ExteriorInfo::PtrVector exteriors;
        PtrVector children;
        
        void buildArrangement( Arr_with_hist_2& arr );
        void buildMetaData( Arr_with_hist_2& arr );
        void generate( Arr_with_hist_2& arr, double minX, double minY, double scale, int& iColour, std::ostream& os );
    };
    AreaInfo::PtrMap m_areaInfoMap;
    AreaInfo::PtrVector m_rootAreaInfos;
    
    void recurse( AreaInfo::Ptr pAreaInfo )
    {
        const ConnectionAnalysis& connections = pAreaInfo->pArea->getConnections();
        
        ContourPoint::ContourPointPtrPairVector interiorConnectionPairs;
        ContourPoint::ContourPointPtrPairVector exteriorConnectionPairs;
        
        //first process all child area interiors
        for( Site::Ptr pSite : pAreaInfo->pArea->getSpaces() )
        {
            if( const Area* pChildArea = dynamic_cast< const Area* >( pSite.get() ) )
            {
                AreaInfo::Ptr pChildAreaInfo =
                    processArea( pChildArea, &connections, interiorConnectionPairs, exteriorConnectionPairs );
                pAreaInfo->children.push_back( pChildAreaInfo );
            }
        }
        
        
        //generate interior connections
        {
            using FCSMap = std::map< const Feature_ContourSegment*, ContourPoint::ContourPointPtrPair >;
            FCSMap fcsMap;
            for( auto& p : interiorConnectionPairs )
            {
                fcsMap.insert( std::make_pair( p.first->getFCS(), p ) );
            }
            const ConnectionAnalysis::ConnectionPairMap& connectionPairs = connections.getConnections();
            for( ConnectionAnalysis::ConnectionPairMap::const_iterator
                i = connectionPairs.begin(), iEnd = connectionPairs.end(); i!=iEnd; ++i )
            {
                const Feature_ContourSegment* pFCSFirst    = i->first.first;
                const Feature_ContourSegment* pFCSSecond   = i->first.second;
                
                FCSMap::iterator iFindFirst     = fcsMap.find( pFCSFirst );
                FCSMap::iterator iFindSecond    = fcsMap.find( pFCSSecond );
                
                VERIFY_RTE( iFindFirst != fcsMap.end() );
                VERIFY_RTE( iFindSecond != fcsMap.end() );
                
                if( iFindFirst != fcsMap.end() &&
                    iFindSecond != fcsMap.end() )
                {
                    
                    ContourPoint::ContourPointPtrPair pairFirst     = iFindFirst->second;
                    ContourPoint::ContourPointPtrPair pairSecond    = iFindSecond->second;
                    
                    ConnectionCurves::Ptr pConnection( new ConnectionCurves );
                    
                    pConnection->m_pFirstStart  = pairFirst.first;
                    pConnection->m_pFirstEnd    = pairFirst.second;
                    pConnection->m_pSecondStart = pairSecond.first;
                    pConnection->m_pSecondEnd   = pairSecond.second;
                        
                    ContourPoint::Vector2D v = 
                        (   pConnection->m_pSecondEnd->getPoint() - 
                            pConnection->m_pFirstStart->getPoint() );
                    v.x /= 2.0f;
                    v.y /= 2.0f;
                    
                    pConnection->m_pFirstMid = ContourPoint::Ptr( 
                        new ContourPoint( pConnection->m_pFirstStart->getArea(), false, 
                            ContourPoint::Start, wykobi::make_point< float >(
                                pConnection->m_pFirstStart->getPoint().x + v.x, 
                                pConnection->m_pFirstStart->getPoint().y + v.y  ), nullptr ) );
                    pConnection->m_pSecondMid = ContourPoint::Ptr( 
                        new ContourPoint( pConnection->m_pSecondStart->getArea(), false, 
                            ContourPoint::Start, wykobi::make_point< float >( 
                                pConnection->m_pFirstEnd->getPoint().x + v.x, 
                                pConnection->m_pFirstEnd->getPoint().y + v.y  ), nullptr ) );
                        
                    pAreaInfo->connectionCurves.push_back( pConnection );
                }
            }
        }
        
        //now construct exterior points WITH connection points
        {
            const ExteriorAnalysis& exteriorAnalysis = pAreaInfo->pArea->getExteriors();
            const ExteriorAnalysis::Exterior::PtrVector& exteriors = exteriorAnalysis.getExteriors();
            
            for( ExteriorAnalysis::Exterior::Ptr pExterior : exteriors )
            {
                ExteriorInfo::Ptr pExteriorInfo( new ExteriorInfo );
                {
                    pExteriorInfo->pArea = pAreaInfo->pArea;
                    pExteriorInfo->pExterior = pExterior.get();
                }
                m_exteriorInfoMap.insert( std::make_pair( pExterior.get(), pExteriorInfo ) );
                pAreaInfo->exteriors.push_back( pExteriorInfo );
                
                //get the absolute contour
                wykobi::polygon< float, 2u > exteriorPolygon = pExterior->getPolygon();
                getAbsoluteOrientedContour( pAreaInfo->pArea->getAbsoluteTransform(), wykobi::Clockwise, exteriorPolygon );
                
                //determine absolute boundaries
                getExteriorBoundaries( exteriorPolygon, exteriorConnectionPairs, 
                    pExteriorInfo->boundaries );
                    
                //produce the absolute exterior contour
                getExteriorContourPoints( connections, pAreaInfo->pArea, exteriorPolygon, 
                    pExteriorInfo->boundaries,
                    pExteriorInfo->exteriorPoints );
                    
                for( const ExteriorBoundary& exteriorBoundary : pExteriorInfo->boundaries )
                {
                    ConnectionCurves::Ptr pConnection( new ConnectionCurves );
                    
                    pConnection->m_pFirstStart  = exteriorBoundary.m_pFirstStart;
                    pConnection->m_pFirstEnd    = exteriorBoundary.m_pFirstEnd;
                    pConnection->m_pSecondStart = exteriorBoundary.m_pSecondStart;
                    pConnection->m_pSecondEnd   = exteriorBoundary.m_pSecondEnd;
                    
                    VERIFY_RTE( pConnection->m_pFirstStart  );
                    VERIFY_RTE( pConnection->m_pFirstEnd    );
                    VERIFY_RTE( pConnection->m_pSecondStart );
                    VERIFY_RTE( pConnection->m_pSecondEnd   );
                    
                    ContourPoint::Vector2D v = 
                        (   pConnection->m_pSecondEnd->getPoint() - 
                            pConnection->m_pFirstStart->getPoint() );
                    v.x /= 2.0f;
                    v.y /= 2.0f;
                    
                    pConnection->m_pFirstMid = ContourPoint::Ptr( 
                        new ContourPoint( pConnection->m_pFirstStart->getArea(), false, 
                            ContourPoint::Start,  wykobi::make_point< float >(
                                pConnection->m_pFirstStart->getPoint().x + v.x, 
                                pConnection->m_pFirstStart->getPoint().y + v.y  ), nullptr ) );
                    pConnection->m_pSecondMid = ContourPoint::Ptr( 
                        new ContourPoint( pConnection->m_pSecondStart->getArea(), false, 
                            ContourPoint::Start,  wykobi::make_point< float >( 
                                pConnection->m_pFirstEnd->getPoint().x + v.x, 
                                pConnection->m_pFirstEnd->getPoint().y + v.y  ), nullptr ) );
                    
                    pExteriorInfo->connectionCurves.push_back( pConnection );
                }
            }
        }
                
        
        for( AreaInfo::Ptr pChildAreaInfo : pAreaInfo->children )
        {
            recurse( pChildAreaInfo );
        }
    }
    
    AreaInfo::Ptr processArea( const Area* pArea, const ConnectionAnalysis* pConnections, 
        ContourPoint::ContourPointPtrPairVector& interiorConnectionPairs, 
        ContourPoint::ContourPointPtrPairVector& exteriorConnectionPairs )
    {
        AreaInfo::Ptr pAreaInfo( new AreaInfo{ pArea } );
        
        //get the relative boundary info
        if( pConnections )
            getInteriorBoundaries( *pConnections, pArea, pAreaInfo->boundaries ); 
            
        const wykobi::polygon< float, 2 > contour = pArea->getContour()->getPolygon();
            
        //using the relative contour and boundaries produce the absolute contour with correct winding
        getInteriorContourPoints( pArea->getAbsoluteTransform(), pConnections, pArea, pAreaInfo->boundaries, contour, 
            pAreaInfo->interiorPoints, interiorConnectionPairs, exteriorConnectionPairs );
        
        m_areaInfoMap.insert( std::make_pair( pArea, pAreaInfo ) );
        
        return pAreaInfo;
    }

public:
    CompilerImpl( const Site::PtrVector& sites )
        :   m_sites( sites )
    {
        //ensure evaluated
        for( Site::Ptr pSite : sites )
        {
            if( Area* pChildArea = dynamic_cast< Area* >( pSite.get() ) )
            {
                Site::EvaluationMode mode;
                DataBitmap data;
                pChildArea->evaluate( mode, data );
            }
        }
        
        {
            ContourPoint::ContourPointPtrPairVector interiorConnectionPairs;
            ContourPoint::ContourPointPtrPairVector exteriorConnectionPairs;;
            for( Site::Ptr pSite : sites )
            {
                if( Area* pChildArea = dynamic_cast< Area* >( pSite.get() ) )
                {
                    AreaInfo::Ptr pAreaInfo = 
                        processArea( pChildArea, nullptr, interiorConnectionPairs, exteriorConnectionPairs );
                    recurse( pAreaInfo );
                    m_rootAreaInfos.push_back( pAreaInfo );
                }
            }
        }
        
        for( AreaInfo::Ptr pAreInfo : m_rootAreaInfos )
        {
            pAreInfo->buildArrangement( m_arr );
        }
        
        //ensure all data pointers are null
        for( Arr_with_hist_2::Halfedge_handle h : m_arr.halfedge_handles() )
        {
            h->set_data( nullptr );
            h->twin()->set_data( nullptr );
        }
        
        for( AreaInfo::Ptr pAreInfo : m_rootAreaInfos )
        {
            pAreInfo->buildMetaData( m_arr );
        }
        
        //extract contours
        
    }
    
    void generateHTML( const boost::filesystem::path& filepath ) const
    {
        std::unique_ptr< boost::filesystem::ofstream > os =
            createNewFileStream( filepath );
            
        double scale = 10.0;
        double  minX = std::numeric_limits< double >::max(), 
                minY = std::numeric_limits< double >::max();
        double  maxX = -std::numeric_limits< double >::max(), 
                maxY = -std::numeric_limits< double >::max();
        for( auto i = m_arr.edges_begin(); i != m_arr.edges_end(); ++i )
        {
            {
                const double x = to_double( i->source()->point().x() );
                const double y = to_double( -i->source()->point().y() );
                if( x < minX ) minX = x;
                if( y < minY ) minY = y;
                if( x > maxX ) maxX = x;
                if( y > maxY ) maxY = y;
            }
            
            {
                const double x = to_double( i->target()->point().x() );
                const double y = to_double( -i->target()->point().y() );
                if( x < minX ) minX = x;
                if( y < minY ) minY = y;
                if( x > maxX ) maxX = x;
                if( y > maxY ) maxY = y;
            }
        }
        const double sizeX = maxX - minX;
        const double sizeY = maxY - minY;
            
        *os << "<!DOCTYPE html>\n";
        *os << "<html>\n";
        *os << "  <head>\n";
        *os << "    <title>Compilation Output</title>\n";
        *os << "  </head>\n";
        *os << "  <body>\n";
        *os << "    <h1>" << filepath.string() << "</h1>\n";
        
        *os << "    <svg width=\"" << 100 + sizeX * scale << "\" height=\"" << 100 + sizeY * scale << "\" >\n";
        *os << "      <defs>\n";
        *os << "      <marker id=\"mid\" markerWidth=\"10\" markerHeight=\"10\" refX=\"0\" refY=\"3\" orient=\"auto\" markerUnits=\"strokeWidth\">\n";
        *os << "      <path d=\"M0,0 L0,6 L9,3 z\" fill=\"#f00\" />\n";
        *os << "      </marker>\n";
        *os << "      </defs>\n";
        *os << "       <text x=\"" << 10 << "\" y=\"" << 10 << 
                         "\" fill=\"green\"  >Vertex Count: " << m_arr.number_of_vertices() << " </text>\n";
        *os << "       <text x=\"" << 10 << "\" y=\"" << 20 << 
                         "\" fill=\"green\"  >Edge Count: " << m_arr.number_of_edges() << " </text>\n";
                         
        for( auto i = m_arr.edges_begin(); i != m_arr.edges_end(); ++i )
        {
            Arr_with_hist_2::Halfedge_handle h = i;
            
            if( i->target()->point() == i->curve().source() )
                h = h->twin();
            
            svgLine( h, minX, minY, scale, "blue", *os );
            
            {
                const double startX = ( to_double(  h->source()->point().x() ) - minX ) * scale;
                const double startY = ( to_double( -h->source()->point().y() ) - minY ) * scale;
                const double endX   = ( to_double(  h->target()->point().x() ) - minX ) * scale;
                const double endY   = ( to_double( -h->target()->point().y() ) - minY ) * scale;
                
                std::ostringstream osText;
                {
                    const void* pData       = h->data();
                    const void* pTwinData   = h->twin()->data();
                    if( const Area* pArea = (const Area*)pData )
                    {
                        osText << "l:" << pArea->getName();
                    }
                    else
                    {
                        osText << "l:";
                    }
                    if( const Area* pArea = (const Area*)pTwinData )
                    {
                        osText << " r:" << pArea->getName();
                    }
                    else
                    {
                        osText << " r:";
                    }
                }
                {
                    float x = startX + ( endX - startX ) / 2.0f;
                    float y = startY + ( endY - startY ) / 2.0f;
                    *os << "       <text x=\"" << x << "\" y=\"" << y << 
                                     "\" fill=\"green\" transform=\"rotate(30 " << x << "," << y << ")\" >" << osText.str() << " </text>\n";
                }
            }
        }
        
        *os << "    </svg>\n";
        *os << "  </body>\n";
        *os << "</html>\n";

    }
  
    void generateOutput( const boost::filesystem::path& filepath ) const
    {
        std::unique_ptr< boost::filesystem::ofstream > os =
            createNewFileStream( filepath );
            
        double scale = 10.0;
        double  minX = std::numeric_limits< double >::max(), 
                minY = std::numeric_limits< double >::max();
        double  maxX = -std::numeric_limits< double >::max(), 
                maxY = -std::numeric_limits< double >::max();
        for( auto i = m_arr.edges_begin(); i != m_arr.edges_end(); ++i )
        {
            {
                const double x = to_double( i->source()->point().x() );
                const double y = to_double( -i->source()->point().y() );
                if( x < minX ) minX = x;
                if( y < minY ) minY = y;
                if( x > maxX ) maxX = x;
                if( y > maxY ) maxY = y;
            }
            
            {
                const double x = to_double( i->target()->point().x() );
                const double y = to_double( -i->target()->point().y() );
                if( x < minX ) minX = x;
                if( y < minY ) minY = y;
                if( x > maxX ) maxX = x;
                if( y > maxY ) maxY = y;
            }
        }
        const double sizeX = maxX - minX;
        const double sizeY = maxY - minY;
            
        *os << "<!DOCTYPE html>\n";
        *os << "<html>\n";
        *os << "  <head>\n";
        *os << "    <title>Compilation Output</title>\n";
        *os << "  </head>\n";
        *os << "  <body>\n";
        *os << "    <h1>" << filepath.string() << "</h1>\n";
        
        *os << "    <svg width=\"" << 100 + sizeX * scale << "\" height=\"" << 100 + sizeY * scale << "\" >\n";
        *os << "      <defs>\n";
        *os << "        <defs>\n";
        *os << "            <marker id=\"mid\" markerWidth=\"10\" markerHeight=\"10\" refX=\"0\" refY=\"3\" orient=\"auto\" markerUnits=\"strokeWidth\">\n";
        *os << "                <path d=\"M0,0 L0,6 L9,3 z\" fill=\"#f00\" />\n";
        *os << "            </marker>\n";
        *os << "        </defs>\n";
        *os << "      </defs>\n";
        
        int iColour = 0;
        for( AreaInfo::Ptr pAreaInfo : m_rootAreaInfos )
        {
            pAreaInfo->generate( m_arr, minX, minY, scale, iColour, *os );
        }

        *os << "    </svg>\n";
        *os << "  </body>\n";
        *os << "</html>\n";
    }

    const Site::PtrVector& m_sites;
    mutable Arr_with_hist_2 m_arr;
};
void Compiler::CompilerImpl::AreaInfo::buildArrangement( Arr_with_hist_2& arr )
{
    {
        VERIFY_RTE( interiorPoints.size() > 2U );
        for( ContourPoint::PtrVector::iterator 
                i       = interiorPoints.begin(),
                iNext   = interiorPoints.begin(),
                iEnd    = interiorPoints.end();
                i!=iEnd; ++i )
        {
            ++iNext;
            if( iNext == iEnd ) iNext = interiorPoints.begin();
            
            ContourPoint::Ptr pPoint = *i;
            ContourPoint::Ptr pNext = *iNext;
            
            Curve_handle ch = CGAL::insert( arr, 
                Segment_2( 
                    Point_2( pPoint->getPoint().x, pPoint->getPoint().y ),
                    Point_2( pNext->getPoint().x, pNext->getPoint().y ) ) );
            pPoint->setCurveHandle( ch );
        }
    }
    {
        for( ConnectionCurves::Ptr pCurve : connectionCurves )
        {
            pCurve->create( arr );
        }
    }
    {
        for( ExteriorInfo::Ptr pExterior : exteriors )
        {
            VERIFY_RTE( pExterior->exteriorPoints.size() > 2U );
            for( ContourPoint::PtrVector::iterator 
                    i       = pExterior->exteriorPoints.begin(),
                    iNext   = pExterior->exteriorPoints.begin(),
                    iEnd    = pExterior->exteriorPoints.end();
                    i!=iEnd; ++i )
            {
                ++iNext;
                if( iNext == iEnd ) iNext = pExterior->exteriorPoints.begin();
                
                ContourPoint::Ptr pPoint = *i;
                ContourPoint::Ptr pNext = *iNext;
                
                Curve_handle ch = CGAL::insert( arr, 
                    Segment_2( 
                        Point_2( pPoint->getPoint().x, pPoint->getPoint().y ),
                        Point_2( pNext->getPoint().x, pNext->getPoint().y ) ) );
                pPoint->setCurveHandle( ch );
            }
            
            for( ConnectionCurves::Ptr pCurve : pExterior->connectionCurves )
            {
                pCurve->create( arr );
            }
        }
    }
    
    for( AreaInfo::Ptr pChildAreaInfo : children )
    {
        pChildAreaInfo->buildArrangement( arr );
    }
}

void Compiler::CompilerImpl::AreaInfo::buildMetaData( Arr_with_hist_2& arr )
{
    for( ExteriorInfo::Ptr pExterior : exteriors )
    {
        for( ConnectionCurves::Ptr pCurve : pExterior->connectionCurves )
        {
            VERIFY_RTE( pCurve->m_pFirstStart->getArea() == pCurve->m_pFirstEnd->getArea() );
            VERIFY_RTE( pCurve->m_pSecondStart->getArea() == pCurve->m_pSecondEnd->getArea() );
            VERIFY_RTE( pCurve->m_pFirstStart->getArea() != pCurve->m_pSecondStart->getArea() );
            pCurve->finish( arr );
        }
    }
    
    for( ConnectionCurves::Ptr pCurve : connectionCurves )
    {
        VERIFY_RTE( pCurve->m_pFirstStart->getArea() == pCurve->m_pFirstEnd->getArea() );
        VERIFY_RTE( pCurve->m_pSecondStart->getArea() == pCurve->m_pSecondEnd->getArea() );
        VERIFY_RTE( pCurve->m_pFirstStart->getArea() != pCurve->m_pSecondStart->getArea() );
        pCurve->finish( arr );
    }
    
    for( AreaInfo::Ptr pChildAreaInfo : children )
    {
        pChildAreaInfo->buildMetaData( arr );
    }
}

void drawEdges( const std::vector< Arr_with_hist_2::Halfedge_const_handle >& edges, 
        double minX, double minY, double scale, int& iColour, std::ostream& os )
{
    const char* pszColour = SVG_COLOURS[ iColour ];
    for( Arr_with_hist_2::Halfedge_const_handle he : edges )
    {
        svgLine( he, minX, minY, scale, pszColour, os );
    }
}

using VertexHandle = Arr_with_hist_2::Vertex_const_handle;
using HalfedgeHandle = Arr_with_hist_2::Halfedge_const_handle;

void Compiler::CompilerImpl::AreaInfo::generate( Arr_with_hist_2& arr, 
            double minX, double minY, double scale, int& iColour, std::ostream& os )
{
    iColour = ( iColour + 1 ) % SVG_COLOURS.size();
    
    std::vector< HalfedgeHandle > path;
    for( auto i = arr.faces_begin(),
        iEnd = arr.faces_end(); i!=iEnd; ++i )
    {
        bool bIsFaceForArea = false;
        path.clear();
        if( !i->is_unbounded() )
        {
            Arr_with_hist_2::Ccb_halfedge_const_circulator iter = i->outer_ccb();
            Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
            do
            {   
                path.push_back( iter );
                if( iter->data() == pArea )
                {
                    bIsFaceForArea = true;
                }
                ++iter;
            }
            while( iter != start );
            
        }
        if( bIsFaceForArea )
        {
            drawEdges( path, minX, minY, scale, iColour, os );
            
            for( Arr_with_hist_2::Hole_const_iterator 
                holeIter = i->holes_begin(),
                holeIterEnd = i->holes_end(); 
                    holeIter != holeIterEnd; ++holeIter )
            {
                path.clear();
                Arr_with_hist_2::Ccb_halfedge_const_circulator iter = *holeIter;
                Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
                do
                {   
                    path.push_back( iter );
                    ++iter;
                }
                while( iter != start );
                
                drawEdges( path, minX, minY, scale, iColour, os );
                
            }
            
        }
    }
    
    for( AreaInfo::Ptr pChildAreaInfo : children )
    {
        pChildAreaInfo->generate( arr, minX, minY, scale, iColour, os );
    }
}
        
Compiler::Compiler( const Site::PtrVector& sites )
    :   m_pPimpl( std::make_shared< CompilerImpl >( sites ) )
{
}

void Compiler::generateHTML( const boost::filesystem::path& filepath ) const
{
    m_pPimpl->generateHTML( filepath );
}
void Compiler::generateOutput( const boost::filesystem::path& filepath ) const
{
    m_pPimpl->generateOutput( filepath );
}

}
