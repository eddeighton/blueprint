#ifndef BUFFER_14_09_2013
#define BUFFER_14_09_2013

#include "common/tick.hpp"
#include "common/assert_verify.hpp"

#include <boost/shared_ptr.hpp>

#include <vector>

namespace Blueprint
{

template< unsigned int PIXEL_SIZE >
class Buffer
{
public:
    typedef std::vector< unsigned char > BitmapType;
    typedef boost::shared_ptr< Buffer > Ptr;

    Buffer( unsigned int uiWidth, unsigned int uiHeight )
        :   m_uiWidth( uiWidth ),
            m_uiHeight( uiHeight ),
            m_buffer( uiWidth * uiHeight * PIXEL_SIZE )
    {
    }
    
    unsigned char* get() { return m_buffer.data(); }
    const unsigned char* get() const { return m_buffer.data(); }
    unsigned int getWidth() const { return m_uiWidth; }
    unsigned int getHeight() const { return m_uiHeight; }
    unsigned int getStride() const { return m_uiWidth * PIXEL_SIZE; }
    unsigned int getSize() const { return m_uiWidth * m_uiWidth * PIXEL_SIZE; }
    const Timing::UpdateTick& getLastUpdateTick() const { return m_lastUpdateTick; }
    void setModified() { m_lastUpdateTick.update(); }

    const unsigned char* getAt( unsigned int uiX, unsigned int uiY ) const
    {
        const unsigned char* pPixel = 0u;

        ASSERT( uiX < m_uiWidth && uiY < m_uiHeight );
        if( uiX < m_uiWidth && uiY < m_uiHeight )
            pPixel = m_buffer.data() + ( uiY * m_uiWidth * PIXEL_SIZE + uiX );
        return pPixel;
    }
    
    void resize( unsigned int uiWidth, unsigned int uiHeight )
    {
        const unsigned int uiRequired = uiWidth * uiHeight * PIXEL_SIZE;
        if( m_buffer.size() < uiRequired )
            m_buffer.resize( uiRequired );
        if( uiWidth != m_uiWidth || uiHeight != m_uiHeight )
            m_lastUpdateTick.update();
        m_uiWidth = uiWidth;
        m_uiHeight = uiHeight;
    }

private:
    unsigned int m_uiWidth, m_uiHeight;
    BitmapType m_buffer;
    Timing::UpdateTick m_lastUpdateTick;
};

typedef Buffer< 1u > NavBitmap;

}

#endif //BUFFER_14_09_2013