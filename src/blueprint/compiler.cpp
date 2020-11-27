
#include "blueprint/compiler.h"
#include "blueprint/connection.h"
#include "blueprint/cgalSettings.h"

#include "common/angle.hpp"
#include "common/file.hpp"

#include "blueprint/cgalSettings.h"

#include <iostream>

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
            m_pFirstStart, m_pFirstEnd, 
            m_pSecondStart, m_pSecondEnd;
        
        Blueprint::Curve_handle m_ch_FirstStart_FirstMid;
        Blueprint::Curve_handle m_ch_FirstEnd_SecondMid;
        Blueprint::Curve_handle m_ch_SecondStart_SecondMid;
        Blueprint::Curve_handle m_ch_SecondEnd_FirstMid;
        Blueprint::Curve_handle m_ch_FirstMid_SecondMid;
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
        
        ContourPoint( PointType _type, Point2D _pt,
                    const Blueprint::Feature_ContourSegment* _pFCS )
            :   m_type( _type ),
                m_pt( _pt ),
                m_pFCS( _pFCS )
        {
        }
        
        PointType getType() const { return m_type; }
        const Point2D& getPoint() const { return m_pt; }
        const Blueprint::Feature_ContourSegment* getFCS() const { return m_pFCS; }
        
        Blueprint::Curve_handle getCurveHandle() const { return m_ch; }
        void setCurveHandle( Blueprint::Curve_handle ch ) { m_ch = ch; }
        
    private:
        PointType m_type;
        Point2D m_pt;
        const Blueprint::Feature_ContourSegment* m_pFCS = nullptr;
        Blueprint::Curve_handle m_ch;
    };
    
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
                points.push_back( ContourPoint::Ptr( new ContourPoint( ContourPoint::Normal, *i, nullptr ) ) );
                
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
                        new ContourPoint( ContourPoint::Start,  boundPair.second->start, boundPair.second->pFCS ) ) );
                    points.push_back( ContourPoint::Ptr( 
                        new ContourPoint( ContourPoint::End,    boundPair.second->end  , boundPair.second->pFCS ) ) );
                    ++totalBoundaries;
                    
                    if( boundPair.second->pFCS->isSegmentExterior() )
                    {
                        newExteriorConnections.push_back(
                            std::make_pair( points[ sz ], points[ sz + 1U ] ) );
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
            if( polyOrientation == wykobi::CounterClockwise )
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
                        const float startfDistance = wykobi::distance( pStart->getPoint(), ptStart );
                            
                        std::cout << "Exterior boundary at: " << 
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
        //ContourPoint::ContourPointPtrPairVector newExteriorConnections;
        {
            int totalBoundaries = 0;
            unsigned int iEdgeCounter = 0;
            for( wykobi::polygon< float, 2 >::const_iterator 
                i = contour.begin(),
                iEnd = contour.end(); i!=iEnd; ++i, ++iEdgeCounter )
            {
                points.push_back( ContourPoint::Ptr( new ContourPoint( ContourPoint::Normal, *i, nullptr ) ) );
                
                std::map< float, ExteriorBoundary* > inSegmentBoundaries;
                for( ExteriorBoundary& boundary : boundaries )
                {
                    if( boundary.edge == iEdgeCounter )
                    {
                        auto result = 
                            inSegmentBoundaries.insert( std::make_pair( boundary.distance, &boundary ) );
                        VERIFY_RTE_MSG( result.second, "Duplicate segment distance error" );
                    }
                }
                
                for( auto& boundPair : inSegmentBoundaries )
                {
                    const std::size_t sz = points.size();
                    points.push_back( ContourPoint::Ptr( 
                        new ContourPoint( ContourPoint::Start,  boundPair.second->start, boundPair.second->pFCS ) ) );
                    points.push_back( ContourPoint::Ptr( 
                        new ContourPoint( ContourPoint::End,    boundPair.second->end  , boundPair.second->pFCS ) ) );
                        
                    {
                        //reverse order of the points
                        boundPair.second->m_pSecondStart = points[ sz + 1U ];
                        boundPair.second->m_pSecondEnd = points[ sz ];
                    }
                        
                    //newExteriorConnections.push_back(
                    //    std::make_pair( points[ sz ], points[ sz + 1U ] ) );
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
    };
    AreaInfo::PtrMap m_areaInfoMap;
    
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
                
                ContourPoint::ContourPointPtrPair pairFirst     = iFindFirst->second;
                ContourPoint::ContourPointPtrPair pairSecond    = iFindSecond->second;
                
                ConnectionCurves::Ptr pConnection( new ConnectionCurves );
                
                pConnection->m_pFirstStart  = pairFirst.first;
                pConnection->m_pFirstEnd    = pairFirst.second;
                pConnection->m_pSecondStart = pairSecond.first;
                pConnection->m_pSecondEnd   = pairSecond.second;
                
                pAreaInfo->connectionCurves.push_back( pConnection );
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
        
        AreaInfo::PtrVector areaInfos;
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
                    areaInfos.push_back( pAreaInfo );
                }
            }
        }
        
        for( AreaInfo::Ptr pAreInfo : areaInfos )
        {
            pAreInfo->buildArrangement( m_arr );
        }
        for( AreaInfo::Ptr pAreInfo : areaInfos )
        {
            pAreInfo->buildMetaData( m_arr );
        }
        
        //extract contours
        
    }
    
    static inline double to_double( const CGAL::Quotient< CGAL::MP_Float >& q )
    {
        return 
            CGAL::INTERN_MP_FLOAT::to_double( q.numerator() ) /
            CGAL::INTERN_MP_FLOAT::to_double( q.denominator() );
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
        
        for( auto i = m_arr.edges_begin(); i != m_arr.edges_end(); ++i )
        {
            const double startX = ( to_double( i->source()->point().x() ) - minX ) * scale;
            const double startY = ( to_double( -i->source()->point().y() ) - minY ) * scale;
            const double endX   = ( to_double( i->target()->point().x() ) - minX ) * scale;
            const double endY   = ( to_double( -i->target()->point().y() ) - minY ) * scale;
            
            std::ostringstream osEdge;
            osEdge << startX << "," << startY << " " << endX << "," << endY;
            
            *os << "       <polyline points=\"" << osEdge.str() << "\" style=\"fill:none;stroke:blue;stroke-width:1\" />\n";
            *os << "       <circle cx=\"" << startX << "\" cy=\"" << startY << "\" r=\"3\" stroke=\"green\" stroke-width=\"1\" fill=\"green\" />\n";
            *os << "       <circle cx=\"" << endX << "\" cy=\"" << endY << "\" r=\"3\" stroke=\"green\" stroke-width=\"1\" fill=\"green\" />\n";
        }
        
        *os << "    </svg>\n";
        *os << "  </body>\n";
        *os << "</html>\n";

    }
  
    const Site::PtrVector& m_sites;
    Arr_with_hist_2 m_arr;
};

void Compiler::CompilerImpl::AreaInfo::buildArrangement( Arr_with_hist_2& arr )
{
    {
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
            ContourPoint::Vector2D v = 
                (   pCurve->m_pSecondEnd->getPoint() - 
                    pCurve->m_pFirstStart->getPoint() );
            v.x /= 2.0f;
            v.y /= 2.0f;
        
            pCurve->m_ch_FirstStart_FirstMid = CGAL::insert( arr, 
                Segment_2( 
                    Point_2( pCurve->m_pFirstStart->getPoint().x,           pCurve->m_pFirstStart->getPoint().y ),
                    Point_2( pCurve->m_pFirstStart->getPoint().x + v.x,     pCurve->m_pFirstStart->getPoint().y + v.y ) ) );
            pCurve->m_ch_FirstEnd_SecondMid = CGAL::insert( arr, 
                Segment_2( 
                    Point_2( pCurve->m_pFirstEnd->getPoint().x,             pCurve->m_pFirstEnd->getPoint().y ),
                    Point_2( pCurve->m_pFirstEnd->getPoint().x + v.x,       pCurve->m_pFirstEnd->getPoint().y + v.y ) ) );
            pCurve->m_ch_SecondStart_SecondMid = CGAL::insert( arr, 
                Segment_2( 
                    Point_2( pCurve->m_pSecondStart->getPoint().x,          pCurve->m_pSecondStart->getPoint().y ),
                    Point_2( pCurve->m_pSecondStart->getPoint().x - v.x,    pCurve->m_pSecondStart->getPoint().y - v.y ) ) );
            pCurve->m_ch_SecondEnd_FirstMid = CGAL::insert( arr, 
                Segment_2( 
                    Point_2( pCurve->m_pSecondEnd->getPoint().x,            pCurve->m_pSecondEnd->getPoint().y ),
                    Point_2( pCurve->m_pSecondEnd->getPoint().x - v.x,      pCurve->m_pSecondEnd->getPoint().y - v.y ) ) );
            pCurve->m_ch_FirstMid_SecondMid = CGAL::insert( arr, 
                Segment_2( 
                    Point_2( pCurve->m_pFirstStart->getPoint().x + v.x,     pCurve->m_pFirstStart->getPoint().y + v.y ),
                    Point_2( pCurve->m_pSecondStart->getPoint().x - v.x,    pCurve->m_pSecondStart->getPoint().y - v.y ) ) );
        }
    }
    {
        for( ExteriorInfo::Ptr pExterior : exteriors )
        {
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
                ContourPoint::Vector2D v = 
                    (   pCurve->m_pSecondEnd->getPoint() - 
                        pCurve->m_pFirstStart->getPoint() );
                v.x /= 2.0f;
                v.y /= 2.0f;
            
                pCurve->m_ch_FirstStart_FirstMid = CGAL::insert( arr, 
                    Segment_2( 
                        Point_2( pCurve->m_pFirstStart->getPoint().x,           pCurve->m_pFirstStart->getPoint().y ),
                        Point_2( pCurve->m_pFirstStart->getPoint().x + v.x,     pCurve->m_pFirstStart->getPoint().y + v.y ) ) );
                pCurve->m_ch_FirstEnd_SecondMid = CGAL::insert( arr, 
                    Segment_2( 
                        Point_2( pCurve->m_pFirstEnd->getPoint().x,             pCurve->m_pFirstEnd->getPoint().y ),
                        Point_2( pCurve->m_pFirstEnd->getPoint().x + v.x,       pCurve->m_pFirstEnd->getPoint().y + v.y ) ) );
                pCurve->m_ch_SecondStart_SecondMid = CGAL::insert( arr, 
                    Segment_2( 
                        Point_2( pCurve->m_pSecondStart->getPoint().x,          pCurve->m_pSecondStart->getPoint().y ),
                        Point_2( pCurve->m_pSecondStart->getPoint().x - v.x,    pCurve->m_pSecondStart->getPoint().y - v.y ) ) );
                pCurve->m_ch_SecondEnd_FirstMid = CGAL::insert( arr, 
                    Segment_2( 
                        Point_2( pCurve->m_pSecondEnd->getPoint().x,            pCurve->m_pSecondEnd->getPoint().y ),
                        Point_2( pCurve->m_pSecondEnd->getPoint().x - v.x,      pCurve->m_pSecondEnd->getPoint().y - v.y ) ) );
                pCurve->m_ch_FirstMid_SecondMid = CGAL::insert( arr, 
                    Segment_2( 
                        Point_2( pCurve->m_pFirstStart->getPoint().x + v.x,     pCurve->m_pFirstStart->getPoint().y + v.y ),
                        Point_2( pCurve->m_pSecondStart->getPoint().x - v.x,    pCurve->m_pSecondStart->getPoint().y - v.y ) ) );
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
    {
        for( ExteriorInfo::Ptr pExterior : exteriors )
        {
        }
    }
    for( AreaInfo::Ptr pChildAreaInfo : children )
    {
        pChildAreaInfo->buildMetaData( arr );
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
}
