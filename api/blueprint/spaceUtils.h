#ifndef SPACEUTILS_15_09_2013
#define SPACEUTILS_15_09_2013

#include "blueprint/geometry.h"
#include "blueprint/rasteriser.h"
#include "blueprint/buffer.h"
#include "blueprint/glyphspec.h"
#include "blueprint/dataBitmap.h"

#include "common/angle.hpp"

#include "clipper.hpp"
#include "wykobi.hpp"

#include <boost/optional.hpp>

#include <string>
#include <cmath>
#include <tuple>

namespace Blueprint
{

static wykobi::point2d< float > calculateOffset( const wykobi::rectangle< float >& aabbBox )
{
    wykobi::point2d< float > ptTopLeft = wykobi::rectangle_corner( aabbBox, 0 );
    ptTopLeft.x = std::floorf( ptTopLeft.x );
    ptTopLeft.y = std::floorf( ptTopLeft.y );
    return ptTopLeft;
}

static wykobi::point2d< float > sizeBuffer( NavBitmap::Ptr& pBuffer, const wykobi::rectangle< float >& aabbBox )
{
    wykobi::point2d< float > ptTopLeft = wykobi::rectangle_corner( aabbBox, 0 );
    ptTopLeft.x = std::floorf( ptTopLeft.x );
    ptTopLeft.y = std::floorf( ptTopLeft.y );
    wykobi::point2d< float > ptBotRight = wykobi::rectangle_corner( aabbBox, 2 );
    ptBotRight.x = std::ceilf( ptBotRight.x );
    ptBotRight.y = std::ceilf( ptBotRight.y );
    const wykobi::vector2d< float > vSize = ptBotRight - ptTopLeft;
    if( !pBuffer ) 
        pBuffer.reset( new NavBitmap( agg::uceil( vSize.x ), agg::uceil( vSize.y ) ) );
    else            
        pBuffer->resize( agg::uceil( vSize.x ), agg::uceil( vSize.y ) );
    return ptTopLeft;
}

typedef std::tuple< unsigned int, float, wykobi::point2d< float > > SegmentIDRatioPointTuple;

static SegmentIDRatioPointTuple findClosestPointOnContour( 
    const wykobi::polygon< float, 2 >& contour, const wykobi::point2d< float >& pt )
{
    float fBest = std::numeric_limits< float >::max();
    wykobi::point2d< float > ptResult = pt;
    unsigned int uiBestIndex = 0u;
    float fBestRatio = 0.0f;
    {
        unsigned int uiIndex = 0u;
        for( wykobi::polygon< float, 2 >::const_iterator 
            i = contour.begin(),
            iNext = contour.begin(),
            iEnd = contour.end(); i!=iEnd; ++i, ++uiIndex )
        {
            ++iNext;
            if( iNext == iEnd )
                iNext = contour.begin();

            const wykobi::point2d< float > ptClosest = 
                wykobi::closest_point_on_segment_from_point( 
                    wykobi::make_segment( *i, *iNext ), pt );
            const float fDistance = wykobi::distance( ptClosest, pt );
            if( fDistance < fBest )
            {
                fBest = fDistance;
                ptResult = ptClosest;
                uiBestIndex = uiIndex;
                fBestRatio = wykobi::distance( *i, ptClosest ) / wykobi::distance( *i, *iNext );
            }
        }
    }

    return SegmentIDRatioPointTuple( uiBestIndex, fBestRatio, ptResult );
}


    
class SegmentIterator
{
public:
    typedef wykobi::polygon< float, 2 > Polygon;
    typedef Polygon::const_iterator Iter;
    typedef wykobi::segment< float, 2 > Segment;
    typedef wykobi::point2d< float > Point;
    typedef wykobi::vector2d< float > Vector;

public:
    SegmentIterator( const Polygon& polygon, unsigned int uiIndex = 0u )
        :   m_polygon( polygon ),
            m_iter( m_polygon.begin() + uiIndex )
    {
        ASSERT( uiIndex < m_polygon.size() );
    }

    bool operator==( const SegmentIterator& cmp ) const
    {
        ASSERT( &m_polygon == &cmp.m_polygon );
        return m_iter == cmp.m_iter;
    }

    Segment segment() const 
    {
        ASSERT( m_polygon.size() > 1u );
        return wykobi::edge< float >( m_polygon, m_iter - m_polygon.begin() );
    }
    const Point& point() const
    {
        ASSERT( m_polygon.size() );
        return *m_iter;
    }
    Vector normal() const
    {
        ASSERT( m_polygon.size() > 1u );
        Iter iNext = m_iter + 1;
        if( iNext == m_polygon.end() )
            iNext = m_polygon.begin();
        return wykobi::normalize( *iNext - *m_iter );
    }

    unsigned int index() const
    {
        return std::distance( m_polygon.begin(), m_iter );
    }

    void operator++()
    {
        if( m_polygon.size() )
        {
            ++m_iter;
            if( m_iter == m_polygon.end() )
                m_iter = m_polygon.begin();
        }
    }
    void operator--()
    {
        if( m_polygon.size() )
        {
            if( m_iter == m_polygon.begin() )
                m_iter = m_polygon.end();
            --m_iter;
        }
    }

private:
    const Polygon& m_polygon;
    Iter m_iter;
};

static float calculateDistanceAroundContour( const wykobi::polygon< float, 2 >& contour, 
                                            unsigned int ui1, float fRatio1, unsigned int ui2, float fRatio2 )
{
    float fResult = 0.0f;
    
    SegmentIterator iter1( contour, ui1 );
    float fDist1 = wykobi::distance( iter1.segment() ) * ::fabsf( fRatio1 );
    const SegmentIterator iter2( contour, ui2 );
    float fDist2 = wykobi::distance( iter2.segment() ) * ::fabsf( fRatio2 );

    if( iter1 == iter2 && fDist1 <= fDist2 )
    {
        fResult = fDist2 - fDist1;
    }
    else
    {
        fResult += wykobi::distance( iter1.segment() ) - fDist1;
        ++iter1;
        while( !(iter1 == iter2) )
        {
            fResult += wykobi::distance( iter1.segment() );
            ++iter1;
        }
        fResult += fDist2;
    }

    const float fPerimeter = wykobi::perimeter( contour );
    return ( fResult < fPerimeter - fResult ) ? fResult : fPerimeter - fResult;
}

static SegmentIterator::Point calculatePointOnPolygon( const wykobi::polygon< float, 2 >& contour, 
                                unsigned int uiStartIndex, float fStartRatio, float fDistance )
{
    SegmentIterator iter( contour, uiStartIndex );
    float fDist = wykobi::distance( iter.segment() ) * ::fabsf( fStartRatio );
    fDistance = -fDistance;
    if( fDistance < 0.0f )
    {
        fDistance = ::fabsf( fDistance );
        while( fDist < fDistance )
        {
            fDistance -= fDist;
            --iter;
            fDist = wykobi::distance( iter.segment() );
        }
        return iter.point() + iter.normal() * ( fDist - fDistance );
    }
    else if( fDistance > 0.0f )
    {
        while( fDist + fDistance > wykobi::distance( iter.segment() ) )
        {
            fDistance -= ( wykobi::distance( iter.segment() ) - fDist );
            ++iter;
            fDist = 0.0f;
        }
        return iter.point() + iter.normal() * ( fDist + fDistance );
    }
    return iter.point();
}

static unsigned int calculateIndexOnPolygon( const wykobi::polygon< float, 2 >& contour, 
                                unsigned int uiStartIndex, float fStartRatio, float fDistance, float& fActualDistance )
{
    SegmentIterator iter( contour, uiStartIndex );
    float fDist = wykobi::distance( iter.segment() ) * ::fabsf( fStartRatio );
    fDistance = -fDistance;
    if( fDistance < 0.0f )
    {
        fDistance = ::fabsf( fDistance );
        while( fDist < fDistance )
        {
            fDistance -= fDist;
            --iter;
            fDist = wykobi::distance( iter.segment() );
        }
        fActualDistance = fDist - fDistance;
    }
    else if( fDistance > 0.0f )
    {
        while( fDist + fDistance > wykobi::distance( iter.segment() ) )
        {
            fDistance -= ( wykobi::distance( iter.segment() ) - fDist );
            ++iter;
            fDist = 0.0f;
        }
        fActualDistance = fDist + fDistance;
    }
    return iter.index();
}
    /*
template< class StrokeType >
static void aggStrokeToClipperPoly( StrokeType& stroke, ClipperLib::Clipper& clipper, ClipperLib::Polygons& solution,
                            float fOffsetX = 0.0f, float fOffsetY = 0.0f, float fScale = 1.0f )
{
    using namespace Logic::Shared;

    //convert to clipper polygon
    ClipperLib::Polygon clipperPoly;
    {
        double x, y;
        unsigned cmd;
        stroke.rewind(0);
        while( true )
        {
            cmd = stroke.vertex(&x, &y);
            if( agg::is_line_to( cmd ) )
            {
                clipperPoly.push_back( 
                    ClipperLib::IntPoint( 
                        static_cast< ClipperLib::long64 >( ( ( x  * fScale ) + fOffsetX ) * CLIPPER_MAG ), 
                        static_cast< ClipperLib::long64 >( ( ( y  * fScale ) + fOffsetY ) * CLIPPER_MAG ) ) );
            }
            else if( agg::is_move_to( cmd ) )
            {
                if( !clipperPoly.empty() ) 
                {
                    clipper.AddPolygon( clipperPoly, ClipperLib::ptSubject );
                    clipperPoly.clear();
                }
                clipperPoly.push_back( 
                    ClipperLib::IntPoint( 
                        static_cast< ClipperLib::long64 >( ( ( x  * fScale ) + fOffsetX ) * CLIPPER_MAG ), 
                        static_cast< ClipperLib::long64 >( ( ( y  * fScale ) + fOffsetY ) * CLIPPER_MAG ) ) );
            }
            else if( agg::is_close( cmd ) )
            {
                if( !clipperPoly.empty() ) 
                {
                    clipperPoly.push_back( clipperPoly.front() );
                    clipper.AddPolygon( clipperPoly, ClipperLib::ptSubject );
                    clipperPoly.clear();
                }
            }
            else if( agg::is_stop( cmd ) )
            {
                if( !clipperPoly.empty() ) 
                {
                    clipper.AddPolygon( clipperPoly, ClipperLib::ptSubject );
                    clipperPoly.clear();
                }
                break;
            }
            else
            {
                throw std::runtime_error( "Unknown vertex command" );
            }
        }
    }

    //run clipper to produce polygon set
    clipper.Execute( ClipperLib::ctUnion, solution, ClipperLib::pftPositive, ClipperLib::pftPositive ); 
}*/


/*
template< class TPathType >
static void generatePathPolygons( const TPathType& path, float fOffsetX = 0.0f, float fOffsetY = 0.0f, float fScale = 0.0f  )
{
    ClipperLib::Clipper clipper;
    ClipperLib::Polygons solution;
    aggStrokeToClipperPoly< StrokeType >( path, clipper, solution, fOffsetX, fOffsetY, fScale );
}*/

    /*
        typedef stl_vertex_source< T, TMemberAccess > PathType;
        typedef agg::conv_bspline< PathType > conv_bspline_type;
    
        conv_bspline_type bspline( path );
        bspline.interpolation_step( fInterpolation > 0.0f ? 1.0f / fInterpolation : 1.0f );

        typedef agg::conv_stroke< conv_bspline_type > conv_stroke_type;
        conv_stroke_type stroke( bspline );
            
        stroke.width( fWidth );
        stroke.line_cap( static_cast< agg::line_cap_e >( cap ) );
        stroke.line_join( static_cast< agg::line_join_e >( join ) );*/
    
/*
static const double CLIPPER_MAG = 1000000.0;

static void offsetSimplePolygon( const wykobi::polygon< float, 2u >& polygon, wykobi::polygon< float, 2u >& result, float fAmt )
{
    ClipperLib::Paths in;
    
    if( polygon.size() != 0U )
    {
        ClipperLib::Path clipperPathIn;
        for( wykobi::polygon< float, 2u >::const_iterator 
            i = polygon.begin(),
            iEnd = polygon.end(); i!=iEnd; ++i )
        {
            clipperPathIn.push_back( 
                ClipperLib::IntPoint( 
                    static_cast< ClipperLib::cInt >( i->x * CLIPPER_MAG ), 
                    static_cast< ClipperLib::cInt >( i->y * CLIPPER_MAG ) ) );
        }

        clipperPathIn.push_back( clipperPathIn.front() );
        in.push_back( clipperPathIn );
        //ClipperLib::OffsetPaths( in, in, fAmt * CLIPPER_MAG, ClipperLib::jtRound, ClipperLib::etClosed );
        
        ClipperLib::ClipperOffset co;// = new ClipperLib::ClipperOffset();
        co.AddPaths( in, ClipperLib::jtSquare, ClipperLib::etClosedPolygon );
        co.Execute( in, fAmt * CLIPPER_MAG );
    }
    
    //ClipperLib::OffsetPaths( in, in, fAmt * CLIPPER_MAG, ClipperLib::jtSquare, ClipperLib::etClosed );
    if( !in.empty() )
    {
        for( ClipperLib::Path::const_iterator 
            j = in[0].begin(), jEnd = in[0].end(); j!=jEnd; ++j )
        {
            result.push_back( 
                wykobi::make_point< float >(static_cast<float>(j->X / CLIPPER_MAG),
                                            static_cast<float>(j->Y / CLIPPER_MAG) ) );
        }
    }
    else
        result = polygon;
}

static void getOuterContours( 
    const std::vector< std::vector< std::pair< float, float > > >& contour, 
    float fWidth,
    std::vector< std::vector< std::pair< float, float > > >& outerContours )
{
    bool bFirst = true;
    for( std::vector< std::vector< std::pair< float, float > > >::const_iterator i = contour.begin(),
        iEnd = contour.end(); i!=iEnd; ++i)
    {
        const std::vector< std::pair< float, float > >& contour = *i;

        wykobi::polygon< float, 2u > poly( contour.size() );
        int c = 0;
        for( auto j = contour.begin(),
            jEnd = contour.end(); j!=jEnd; ++j, ++c )
        {
            poly[ c ] = wykobi::make_point< float >( j->first, j->second );
        }

        wykobi::polygon< float, 2u > polyExtruded;
        offsetSimplePolygon( poly, polyExtruded, ( bFirst ) ? fWidth : -fWidth );
        std::vector< std::pair< float, float > > extrudedContour;
        for( auto j = polyExtruded.begin(),
            jEnd = polyExtruded.end(); j!=jEnd; ++j )
        {
            extrudedContour.push_back( std::make_pair( j->x, j->y ) );
        }
        outerContours.push_back( extrudedContour );
        bFirst = false;
    }
}*/

//typedef std::vector< wykobi::point2d< float > > PointVector;
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

}

#endif //SPACEUTILS_15_09_2013