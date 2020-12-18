
#ifndef SVG_UTILS_15_DEC_2020
#define SVG_UTILS_15_DEC_2020

#include "cgalSettings.h"

#include "boost/filesystem/path.hpp"


namespace Blueprint
{

    using EdgeVector = std::vector< Arrangement::Halfedge_const_handle >;
    using EdgeVectorVector = std::vector< EdgeVector >;
    
    struct SVGStyle
    {
        bool bDots = true;
        bool bArrows = true;
    };
    
    void generateHTML( const boost::filesystem::path& filepath,
            const Arrangement& arr,
            const EdgeVectorVector& edgeGroups,
            const SVGStyle& style );

}

#endif //SVG_UTILS_15_DEC_2020