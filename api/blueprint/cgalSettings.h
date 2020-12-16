
#ifndef CGAL_SETINGS_26_NOV_2020
#define CGAL_SETINGS_26_NOV_2020


#include <CGAL/Cartesian.h>
//#include <CGAL/Simple_cartesian.h>
#include <CGAL/Exact_rational.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_on_surface_with_history_2.h>
#include <CGAL/Arrangement_with_history_2.h>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arr_simple_point_location.h>
//#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Boolean_set_operations_2.h>


namespace Blueprint
{
    //typedef double                                                  Number_type;
    //typedef CGAL::Simple_cartesian< Number_type >                   Kernel;
    typedef CGAL::Quotient<CGAL::MP_Float> Number_type;
    //typedef CGAL::Cartesian< CGAL::Exact_rational >                 Kernel;
    typedef CGAL::Cartesian< Number_type >                          Kernel;
    //typedef CGAL::Exact_predicates_exact_constructions_kernel Kernel;
    typedef Kernel::Point_2                            Point_2;
    typedef CGAL::Polygon_2<Kernel>                    Polygon_2;
    typedef CGAL::Polygon_with_holes_2<Kernel>         Polygon_with_holes_2;
    typedef CGAL::Arr_segment_traits_2< Kernel >                    Traits_2;
    //typedef Traits_2::Point_2                                       Point_2;
    typedef Traits_2::X_monotone_curve_2                            Segment_2;

    struct DefaultedBool
    {
        DefaultedBool(){}
        DefaultedBool( bool b ) : m_bValue( b ) {}
        
        bool get() const { return m_bValue; }
        void set( bool b ) { m_bValue = b; }
        
    private:
        bool m_bValue = false;
    };
    typedef CGAL::Arr_extended_dcel< Traits_2, DefaultedBool, DefaultedBool, DefaultedBool > Dcel;
    typedef CGAL::Arrangement_with_history_2< Traits_2, Dcel >      Arr_with_hist_2;
    typedef Arr_with_hist_2::Curve_handle                           Curve_handle;
    typedef CGAL::Arr_simple_point_location< Arr_with_hist_2 >      Point_location;

}

#endif //CGAL_SETINGS_26_NOV_2020