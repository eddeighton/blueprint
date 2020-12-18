#ifndef RASTERISER_21_09_2013
#define RASTERISER_21_09_2013

#include "blueprint/buffer.h"

#include "agg_basics.h"
//#include "agg_pixfmt_rgba.h"
#include "agg_pixfmt_gray.h"
#include "agg_rendering_buffer.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_scanline_p.h"
#include "agg_renderer_scanline.h"
#include "agg_path_storage.h"
#include "agg_conv_stroke.h"
#include "agg_trans_affine.h"
#include "agg_conv_transform.h"

#include <vector>

namespace Blueprint
{

class Rasteriser
{
public:
    using Float = double;
    
    //typedef agg::pixfmt_bgra32                          PixelFormatType;
    typedef agg::pixfmt_gray8                           PixelFormatType;
    typedef PixelFormatType::color_type                 ColourType;
    typedef agg::renderer_base< PixelFormatType >       RendererBaseType;
    typedef agg::scanline_p8                            ScanlineType;
    typedef agg::rasterizer_scanline_aa<>               RasterizerType;
public:
    Rasteriser( NavBitmap::Ptr pBuffer, bool bClear = true )
        :   m_pBuffer( pBuffer ),
            m_renderBuffer( m_pBuffer->get(), m_pBuffer->getWidth(), m_pBuffer->getHeight(), m_pBuffer->getStride() ),
            m_pixelFormater( m_renderBuffer ),
            m_renderer( m_pixelFormater )
    {
        if( bClear )
            m_renderer.clear( ColourType( 0u ) );
    }

    template< class T >
    void renderPath( T& path, const ColourType& colour, Float fGamma = 0.0f )
    {
        RasterizerType ras;
        ras.gamma( agg::gamma_threshold( fGamma ) );
        ras.add_path( path );
        agg::render_scanlines_aa_solid( ras, m_scanLine, m_renderer, colour );
    }

    template< class T >
    void renderPath( T& path, const ColourType& colour, Float fX, Float fY, Float fGamma = 0.0f )
    {
        agg::trans_affine transform;
        transform *= agg::trans_affine_translation( fX, fY );
        renderPath( agg::conv_transform< T >( path, transform ), colour, fGamma );
    }

    void setPixel( int x, int y, const ColourType& colour )
    {
        m_renderer.copy_pixel( x, y, colour );
    }

    inline const ColourType& getPixel( int x, int y ) const
    {
        return m_renderer.pixel( x, y );
    }

    NavBitmap::Ptr getBuffer() { return m_pBuffer; }

private:
    NavBitmap::Ptr m_pBuffer;
    agg::rendering_buffer m_renderBuffer;
    PixelFormatType m_pixelFormater;
    RendererBaseType m_renderer;
    ScanlineType m_scanLine;
};

}

#endif //RASTERISER_21_09_2013