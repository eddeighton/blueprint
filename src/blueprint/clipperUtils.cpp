
#include "blueprint/clipperUtils.h"

#include <algorithm>
#include <limits>

namespace
{
    //static const double CLIPPER_MAG = std::numeric_limits< int64_t >::max() / 2;
    static const double CLIPPER_MAG = 1000000.0;
}

namespace Blueprint
{

void toClipperPoly( const Polygon2D& polygon, int iOrientation, ClipperLib::Path& path )
{
    for( const Point2D& pt : polygon )
    {
        path.push_back( 
            ClipperLib::IntPoint( 
                static_cast< ClipperLib::cInt >( pt.x * CLIPPER_MAG ), 
                static_cast< ClipperLib::cInt >( pt.y * CLIPPER_MAG ) ) ); 
    }
    if( wykobi::polygon_orientation( polygon ) != iOrientation )
        std::reverse( path.begin(), path.end() );
}

void toClipperPoly( const Polygon2D& polygon, const Matrix& transform, int iOrientation, ClipperLib::Path& path )
{
    Polygon2D temp = polygon;
    for( Point2D& pt : temp )
    {
        transform.transform( pt.x, pt.y );
        path.push_back( 
            ClipperLib::IntPoint( 
                static_cast< ClipperLib::cInt >( pt.x * CLIPPER_MAG ), 
                static_cast< ClipperLib::cInt >( pt.y * CLIPPER_MAG ) ) ); 
    }
    if( wykobi::polygon_orientation( polygon ) != iOrientation )
        std::reverse( path.begin(), path.end() );
}

void fromClipperPoly( const ClipperLib::Path& result, Polygon2D& polygon )
{
    polygon.clear();
    for( ClipperLib::Path::const_iterator 
        j = result.begin(), jEnd = result.end(); j!=jEnd; ++j )
    {
        polygon.push_back( 
            wykobi::make_point< float >( static_cast<float>(j->X / CLIPPER_MAG),
                                         static_cast<float>(j->Y / CLIPPER_MAG) ) );
    }
}

void fromClipperPolys( const ClipperLib::Paths& result, Polygon2D& polygon )
{
    polygon.clear();
    for( const auto& r : result )
    {
        for( ClipperLib::Path::const_iterator 
            j = r.begin(), jEnd = r.end(); j!=jEnd; ++j )
        {
            polygon.push_back( 
                wykobi::make_point< float >( static_cast<float>(j->X / CLIPPER_MAG),
                                             static_cast<float>(j->Y / CLIPPER_MAG) ) );
        }
    } 
}

void extrudePoly( const ClipperLib::Path& inputPolygon, float fAmount, ClipperLib::Paths& result )
{
    //ClipperLib::jtSquare
    ClipperLib::ClipperOffset co;
    co.MiterLimit = 100.0;
    co.AddPath( inputPolygon, ClipperLib::jtMiter, ClipperLib::etClosedPolygon );
    co.Execute( result, fAmount * CLIPPER_MAG );
}

bool unionAndClipPolygons( 
        const ClipperLib::Paths& polygons, 
        const ClipperLib::Path& clipPolygon, 
        ClipperLib::Paths& results )
{
    ClipperLib::Clipper unionClipper;
    for( const ClipperLib::Path& poly : polygons )
    {
        if( !unionClipper.AddPath( poly, ClipperLib::ptSubject, true ) )
        {
            return false;
        }
    }
    
    ClipperLib::Paths unionPaths;
    if( unionClipper.Execute( ClipperLib::ctUnion, unionPaths, 
        ClipperLib::pftPositive, ClipperLib::pftPositive ) )
    {
        ClipperLib::Clipper interiorClip;
        if( interiorClip.AddPaths( unionPaths, ClipperLib::ptSubject, true ) )
        {
            if( interiorClip.AddPath( clipPolygon, ClipperLib::ptClip, true ) )
            {
                if( interiorClip.Execute( ClipperLib::ctIntersection, results, 
                    ClipperLib::pftPositive, ClipperLib::pftPositive ) )
                {
                    return true;
                }
            }
        }
    }
        
    return false;
}
}