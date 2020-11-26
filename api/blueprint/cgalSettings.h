
#ifndef CGAL_SETINGS_26_NOV_2020
#define CGAL_SETINGS_26_NOV_2020

#include <CGAL/Cartesian.h>
#include <CGAL/Exact_rational.h>
#include <CGAL/Arr_segment_traits_2.h>

#include <CGAL/Arrangement_on_surface_with_history_2.h>
#include <CGAL/Arrangement_with_history_2.h>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arr_simple_point_location.h>

namespace Blueprint
{
typedef CGAL::Quotient<CGAL::MP_Float> Number_type;

//typedef CGAL::Cartesian< CGAL::Exact_rational >                 Kernel;
typedef CGAL::Cartesian< Number_type >                          Kernel;
typedef CGAL::Arr_segment_traits_2< Kernel >                    Traits_2;
typedef Traits_2::Point_2                                       Point_2;
typedef Traits_2::X_monotone_curve_2                            Segment_2;

typedef CGAL::Arr_extended_dcel< Traits_2, int, int, int>       Dcel;

typedef CGAL::Arrangement_with_history_2< Traits_2, Dcel >      Arr_with_hist_2;

typedef Arr_with_hist_2::Curve_handle                           Curve_handle;
typedef CGAL::Arr_simple_point_location< Arr_with_hist_2 >      Point_location;


}

#endif //CGAL_SETINGS_26_NOV_2020