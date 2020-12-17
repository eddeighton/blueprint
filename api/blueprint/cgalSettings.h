
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
    //typedef double                                                Number_type;
    //typedef CGAL::MP_Float                                        Number_type;
    typedef CGAL::Quotient< CGAL::MP_Float >                        Number_type;
    //typedef CGAL::Quotient< double >                              Number_type;
    //typedef CGAL::Exact_rational                                  Number_type;
    
    //typedef CGAL::Cartesian< Number_type >                        Kernel;
    typedef CGAL::Simple_cartesian< Number_type >                   Kernel;
    //typedef CGAL::Exact_predicates_exact_constructions_kernel     Kernel;
    //typedef CGAL::Exact_predicates_inexact_constructions_kernel   Kernel;
    
    typedef Kernel::Aff_transformation_2                            Transform_2;
    typedef Kernel::Line_2                                          Line_2;
    typedef Kernel::Point_2                                         Point_2;
    typedef Kernel::Ray_2                                           Ray_2;
    typedef Kernel::Segment_2                                       Segment_2;
    typedef Kernel::Vector_2                                        Vector_2;
    typedef Kernel::Iso_rectangle_2                                 Rect_2;
    typedef CGAL::Bbox_2 	                                        Bbox_2;
    typedef Kernel::Circle_2                                        Circle_2;
    typedef Kernel::Direction_2                                     Direction_2;
    
    typedef CGAL::Polygon_2< Kernel >                               Polygon_2;
    typedef CGAL::Polygon_with_holes_2< Kernel >                    Polygon_with_holes_2;
    typedef CGAL::Arr_segment_traits_2< Kernel >                    Traits;
    typedef Traits::X_monotone_curve_2                              Curve_2;

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
    typedef CGAL::Arrangement_with_history_2< Traits, Dcel >      Arr_with_hist_2;
    typedef Arr_with_hist_2::Curve_handle                           Curve_handle;
    typedef CGAL::Arr_simple_point_location< Arr_with_hist_2 >      Point_location;

}

#endif //CGAL_SETINGS_26_NOV_2020