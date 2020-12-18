#ifndef SPACEUTILS_15_09_2013
#define SPACEUTILS_15_09_2013

#include "blueprint/cgalSettings.h"
#include "blueprint/site.h"

namespace Blueprint
{
    
    namespace Utils
    {

        Polygon getDefaultPolygon();
        std::size_t getClosestPoint( const Polygon& poly, const Point& pt );
        void getSelectionBounds( const std::vector< Site* >& sites, Rect& transformBounds );

    }
        
}

#endif //SPACEUTILS_15_09_2013