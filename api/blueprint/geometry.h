
#ifndef GEOMETRY_TYPES_01_DEC_2020
#define GEOMETRY_TYPES_01_DEC_2020

#include "wykobi.hpp"

#include "common/angle.hpp"

#include <cmath>

namespace Blueprint
{
    using Angle8Traits  = Math::Angle< 8 >;
    using Angle8        = Angle8Traits::Value;
    
    using Point2D       = wykobi::point2d< float >;
    using Polygon2D     = wykobi::polygon< float, 2 >;
    using Rect2D        = wykobi::rectangle< float >;
    using Segment2D     = wykobi::segment< float, 2 >;
    using Vector2D      = wykobi::vector2d< float >;
    
    struct Map_FloorAverage
    {
        float operator()( float fValue ) const
        {
            return std::floorf( 0.5f + fValue );
        }
    };

    struct Map_FloorAverageMin
    {
        const float fMin;
        Map_FloorAverageMin( float _fMin ) : fMin( _fMin ) {}
        float operator()( float fValue ) const
        {
            return std::max( fMin, std::floorf( 0.5f + fValue ) );
        }
    };

}

#endif //GEOMETRY_TYPES_01_DEC_2020