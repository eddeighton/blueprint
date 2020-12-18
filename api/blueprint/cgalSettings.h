
#ifndef CGAL_SETINGS_26_NOV_2020
#define CGAL_SETINGS_26_NOV_2020


#include <CGAL/Cartesian.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Vector_2.h>
#include <CGAL/Ray_2.h>
#include <CGAL/Line_2.h>
#include <CGAL/Iso_rectangle_2.h>
//#include <CGAL/Simple_cartesian.h>
#include <CGAL/Exact_rational.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_on_surface_with_history_2.h>
#include <CGAL/Arrangement_with_history_2.h>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arr_simple_point_location.h>
#include <CGAL/Boolean_set_operations_2.h>

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
//#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

namespace Blueprint
{
    using Float = double;

    //typedef double                                                Number_type;
    //typedef CGAL::MP_Float                                        Number_type;
    typedef CGAL::Quotient< CGAL::MP_Float >                        Number_type;
    //typedef CGAL::Quotient< double >                              Number_type;
    //typedef CGAL::Exact_rational                                  Number_type;
    
    //typedef CGAL::Cartesian< Number_type >                        Kernel;
    typedef CGAL::Simple_cartesian< Number_type >                   Kernel;
    //typedef CGAL::Exact_predicates_exact_constructions_kernel     Kernel;
    //typedef CGAL::Exact_predicates_inexact_constructions_kernel   Kernel;
    
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

    struct DefaultedBool
    {
        DefaultedBool(){}
        DefaultedBool( bool b ) : m_bValue( b ) {}
        bool get() const { return m_bValue; }
        void set( bool b ) { m_bValue = b; }
    private:
        bool m_bValue = false;
    };
    
    typedef CGAL::Arr_extended_dcel< Traits, DefaultedBool, DefaultedBool, DefaultedBool > Dcel;
    typedef CGAL::Arrangement_with_history_2< Traits, Dcel >        Arrangement;
    typedef Arrangement::Curve_handle                               Curve_handle;
    typedef CGAL::Arr_simple_point_location< Arrangement >          Point_location;

}

#endif //CGAL_SETINGS_26_NOV_2020