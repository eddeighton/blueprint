
#ifndef CLIPPER_UTILS_01_DEC_2020
#define CLIPPER_UTILS_01_DEC_2020

#include "blueprint/geometry.h"

#include "clipper.hpp"

namespace Blueprint
{
static const double CLIPPER_MAG = 10000000.0;

inline void toClipperPoly( const Polygon2D& polygon, ClipperLib::Path& path )
{
    for( const Point2D& pt : polygon )
    {
        path.push_back( 
            ClipperLib::IntPoint( 
                static_cast< ClipperLib::cInt >( pt.x * CLIPPER_MAG ), 
                static_cast< ClipperLib::cInt >( pt.y * CLIPPER_MAG ) ) ); 
    }
}

inline void fromClipperPoly( const ClipperLib::Path& result, Polygon2D& polygon )
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
inline void fromClipperPolys( const ClipperLib::Paths& result, Polygon2D& polygon )
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

inline void extrudePoly( const ClipperLib::Path& inputPolygon, float fAmount, ClipperLib::Paths& result )
{
    //ClipperLib::jtSquare
    ClipperLib::ClipperOffset co;
    co.MiterLimit = 1000.0;
    co.AddPath( inputPolygon, ClipperLib::jtMiter, ClipperLib::etClosedPolygon );
    co.Execute( result, fAmount * CLIPPER_MAG );
}

inline bool unionAndClipPolygons( 
        const ClipperLib::Paths& polygons, 
        const ClipperLib::Path& clipPolygon, 
        ClipperLib::Paths& results )
{
    ClipperLib::Clipper unionClipper;
    if( unionClipper.AddPaths( polygons, ClipperLib::ptClip, true ) )
    {
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
    }
    return false;
}
              
/*for( const ClipperLib::Path& unionPath : clippedUnionPaths )
{
    Exterior::Ptr pExterior( new Exterior );
    m_exteriors.push_back( pExterior );
    pExterior->m_areas.assign( areas.begin(), areas.end() );
            
    for( ClipperLib::Path::const_iterator 
        j = unionPath.begin(), jEnd = unionPath.end(); j!=jEnd; ++j )
    {
        pExterior->m_polygon.push_back( 
            wykobi::make_point< float >( static_cast<float>(j->X / CLIPPER_MAG),
                                         static_cast<float>(j->Y / CLIPPER_MAG) ) );
    }
}*/

}

#endif //CLIPPER_UTILS_01_DEC_2020