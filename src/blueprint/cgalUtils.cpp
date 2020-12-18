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
        
        std::size_t getClosestPoint( const Polygon& poly, const Point& pt )
        {
            if( poly.size() < 2U ) return 0U;
            
            std::vector< double > distances;
            {
                for( const auto& p : poly )
                {
                    const double db = CGAL::to_double( ( p - pt ).squared_length() );
                    distances.push_back( db );
                }
            }
            const std::size_t szSize = distances.size();
        
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
                        
                        //which is closer the left or right point?
                        if( distances[ ( szSize + ui - 1U ) % szSize ] > 
                            distances[ ( szSize + ui + 1U ) % szSize ] )
                        {
                            uiLowest = ( szSize + ui + 1U ) % szSize;
                        }
                        else
                        {
                            uiLowest = ui;
                        }
                        
                    }
                    ++ui;
                }
            }
            return uiLowest;
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
            transformBounds = CGAL::bbox_2( contours.begin(), contours.end() );
        }
    }
}

