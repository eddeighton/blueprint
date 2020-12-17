
#ifndef CLIPPER_UTILS_01_DEC_2020
#define CLIPPER_UTILS_01_DEC_2020

#include "blueprint/geometry.h"
#include "blueprint/transform.h"

#include "clipper.hpp"

namespace Blueprint
{

void toClipperPoly( const Polygon2D& polygon, int iOrientation, ClipperLib::Path& path );
void toClipperPoly( const Polygon2D& polygon, const Matrix& transform, int iOrientation, ClipperLib::Path& path );
void fromClipperPoly( const ClipperLib::Path& result, Polygon2D& polygon );
void fromClipperPolys( const ClipperLib::Paths& result, Polygon2D& polygon );

void extrudePoly( const ClipperLib::Path& inputPolygon, float fAmount, ClipperLib::Paths& result );

bool unionAndClipPolygons( 
        const ClipperLib::Paths& polygons, 
        const ClipperLib::Path& clipPolygon, 
        ClipperLib::Paths& results );

}

#endif //CLIPPER_UTILS_01_DEC_2020