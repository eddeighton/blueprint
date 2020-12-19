
#ifndef CGAL_SETINGS_26_NOV_2020
#define CGAL_SETINGS_26_NOV_2020

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
//#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
//#include <CGAL/Simple_cartesian.h>
//#include <CGAL/Exact_rational.h>

#include <CGAL/Cartesian.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Vector_2.h>
#include <CGAL/Ray_2.h>
#include <CGAL/Line_2.h>
#include <CGAL/Iso_rectangle_2.h>

#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_on_surface_with_history_2.h>
#include <CGAL/Arrangement_with_history_2.h>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arr_simple_point_location.h>

#include <CGAL/IO/Arr_text_formatter.h>
#include <CGAL/IO/Arr_iostream.h>

#include <CGAL/create_offset_polygons_2.h>
#include <CGAL/Boolean_set_operations_2.h>

#include <ostream>
#include <istream>

namespace Blueprint
{
    struct DefaultedBool
    {
        DefaultedBool(){}
        DefaultedBool( bool b ) : m_bValue( b ) {}
        bool get() const { return m_bValue; }
        void set( bool b ) { m_bValue = b; }
    private:
        bool m_bValue = false;
    };
}

namespace CGAL
{
    inline std::ostream& operator<<( std::ostream& os, const Blueprint::DefaultedBool& defBool)
    {
        return os << defBool.get();
    }

    inline std::istream& operator>>( std::istream& is, Blueprint::DefaultedBool& defBool )
    {
        bool bValue;
        is >> bValue;
        defBool.set( bValue );
        return is;
    }
}


namespace Blueprint
{
    using Float = double;

    //typedef double                                                Number_type;
    //typedef CGAL::MP_Float                                        Number_type;
    //typedef CGAL::Quotient< CGAL::MP_Float >                      Number_type;
    //typedef CGAL::Quotient< double >                              Number_type;
    //typedef CGAL::Exact_rational                                  Number_type;
    
    //typedef CGAL::Cartesian< Number_type >                        Kernel;
    //typedef CGAL::Simple_cartesian< Number_type >                 Kernel;
    //typedef CGAL::Exact_predicates_inexact_constructions_kernel   Kernel;
    typedef CGAL::Exact_predicates_exact_constructions_kernel       Kernel;
    
    typedef Kernel::Aff_transformation_2                            Transform;
    typedef Kernel::Line_2                                          Line;
    typedef Kernel::Point_2                                         Point;
    typedef Kernel::Ray_2                                           Ray;
    typedef Kernel::Segment_2                                       Segment;
    typedef Kernel::Vector_2                                        Vector;
    typedef Kernel::Iso_rectangle_2                                 Rect;
    typedef CGAL::Bbox_2 	                                        Bbox;
    typedef Kernel::Circle_2                                        Circle;
    typedef Kernel::Direction_2                                     Direction;
    
    typedef CGAL::Polygon_2< Kernel >                               Polygon;
    typedef CGAL::Polygon_with_holes_2< Kernel >                    Polygon_with_holes;
    typedef CGAL::Arr_segment_traits_2< Kernel >                    Traits;
    typedef Traits::X_monotone_curve_2                              Curve;
    
    typedef CGAL::Arr_extended_dcel< Traits, DefaultedBool, DefaultedBool, DefaultedBool > Dcel;
    typedef CGAL::Arrangement_with_history_2< Traits, Dcel >        Arrangement;
    typedef Arrangement::Curve_handle                               Curve_handle;
    typedef CGAL::Arr_simple_point_location< Arrangement >          Point_location;
    typedef CGAL::Arr_extended_dcel_text_formatter< Arrangement >   Formatter;
}


#endif //CGAL_SETINGS_26_NOV_2020