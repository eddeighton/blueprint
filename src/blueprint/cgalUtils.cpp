#include "blueprint/cgalUtils.h"

#include <limits>

namespace Blueprint
{
    
    namespace Utils
    {

        Polygon getDefaultPolygon()
        {
            Polygon defaultPolygon;
            {
                const double d = 16.0;
                defaultPolygon.push_back( Point( -d, -d ) );
                defaultPolygon.push_back( Point(  d, -d ) );
                defaultPolygon.push_back( Point(  d,  d ) );
                defaultPolygon.push_back( Point( -d,  d ) );
            }
            return defaultPolygon;
        }
        
        Point getClosestPointOnSegment( const Segment& segment, const Point& pt )
        {
            Kernel::Construct_projected_point_2 project;
            
            const Point projectedPoint = project( segment.supporting_line(), pt ); 
            
            if( segment.has_on( projectedPoint ) )
            {
                return Point( projectedPoint.x(), projectedPoint.y() );
            }
            else
            {
                if( CGAL::compare_distance_to_point( projectedPoint, segment[0], segment[1] ) == CGAL::SMALLER )
                {
                    return segment[ 0 ];
                }
                else
                {
                    return segment[ 1 ];
                }
            }
        }
        
        std::size_t getClosestPoint( const Polygon& poly, const Point& pt )
        {
            if( poly.size() < 2U ) return 0U;
            
            //for each line segment find the closest point to the input point and record distance
            const std::size_t szSize = poly.size();
            std::vector< double > distances;
            {
                for( std::size_t sz = 0U; sz != szSize; ++sz )
                {
                    const Point& ptCur = poly[ sz ];
                    const Point& ptNext = poly[ ( sz + 1U ) % szSize ];
                    const Point ptClosest = getClosestPointOnSegment( Segment( ptCur, ptNext ), pt );
                    const double db = CGAL::to_double( ( ptClosest - pt ).squared_length() );
                    distances.push_back( db );
                }
            }
        
            //now find the smallest distance
            std::size_t uiLowest = 0U;
            {
                double dbLowest = std::numeric_limits< double >::max();
                std::size_t ui = 0U;
                for( const auto& p : poly )
                {
                    const double db = distances[ ui ];
                    if( db < dbLowest )
                    {
                        dbLowest = db;
                        uiLowest = ui;
                    }
                    ++ui;
                }
            }
            return ( szSize + uiLowest + 1U ) % szSize; //off by one
        }
        
        void getSelectionBounds( const std::vector< Site* >& sites, Rect& transformBounds )
        {
            std::vector< Polygon > contours;
            for( const Site* pSite : sites )
            {
                Polygon poly = pSite->getContourPolygon();
                for( auto& p : poly )
                    p = pSite->getTransform()( p );
                contours.push_back( poly );
            }
            if( !contours.empty() )
            {
                transformBounds = CGAL::bbox_2( contours.begin(), contours.end() );
            }
        }
    }
}

