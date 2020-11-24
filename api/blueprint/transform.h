
#ifndef TRANSFORM_07_NOV_2020
#define TRANSFORM_07_NOV_2020

#include "common/angle.hpp"

#include "ed/nodeio.hpp"

#include <string>
#include <sstream>

namespace Blueprint
{
    
class Matrix
{
public:
    Matrix(){}
    Matrix( float x, float y )
        :   m_x( x ),
            m_y( y )
    {}
    
    inline float M11() const { return m11; }
    inline float M12() const { return m12; }
    inline float M21() const { return m21; }
    inline float M22() const { return m22; }
    inline float X()   const { return m_x; }
    inline float Y()   const { return m_y; }
    
    inline void transform( float& x, float& y ) const
    {
        float xTemp = x;
        float yTemp = y;
        
        x = ( m11 * xTemp ) + ( m21 * yTemp ) + m_x;
        y = ( m12 * xTemp ) + ( m22 * yTemp ) + m_y;
    }
    
    inline void transform( Matrix& matrix ) const
    {
        const float n11 = matrix.m11, n12 = matrix.m12, 
                    n21 = matrix.m21, n22 = matrix.m22;
            
        matrix.m11 = ( n11 * m11 ) + ( n21 * m12 );
        matrix.m12 = ( n12 * m11 ) + ( n22 * m12 );
        matrix.m21 = ( n11 * m21 ) + ( n21 * m22 );
        matrix.m22 = ( n12 * m21 ) + ( n22 * m22 );
        
        matrix.m_x += m_x;
        matrix.m_y += m_y;
    }
    
protected:
    float m_x = 0.0f, m_y = 0.0f;
    float m11 = 1.0f, m12 = 0.0f, 
          m21 = 0.0f, m22 = 1.0f;
};
    
class Transform : public Matrix
{
public:
    Transform()
    {
    }
    
    Transform( float x, float y, Math::Angle< 8 >::Value angle, bool bMirrorX, bool bMirrorY )
        :   Matrix( x, y ),
            m_angle( angle ),
            m_bMirrorX( bMirrorX ),
            m_bMirrorY( bMirrorY )
    {
        updateMatrix();
    }
    
    inline Math::Angle< 8 >::Value Angle() const { return m_angle; }
    inline bool MirrorX() const { return m_bMirrorX; }
    inline bool MirrorY() const { return m_bMirrorY; }
    inline bool isWindingInverted() const { return ( m_bMirrorX && !m_bMirrorY ) || ( !m_bMirrorX && m_bMirrorY ); }
    
    void setTranslation( float x, float y )
    {
        m_x = x;
        m_y = y;
        updateMatrix();
    }
    
    void translateBy( float x, float y )
    {
        m_x += x;
        m_y += y;
        updateMatrix();
    }
    
    void rotateLeft()
    {
        m_angle = Math::rotate< Math::Angle< 8 > >( m_angle, 1 );
        updateMatrix();
    }
    
    void rotateRight()
    {
        m_angle = Math::rotate< Math::Angle< 8 > >( m_angle, -1 );
        updateMatrix();
    }
    
    void flipHorizontally()
    {
        m_bMirrorX = !m_bMirrorX;
        updateMatrix();
    }
    
    void flipVertically()
    {
        m_bMirrorY = !m_bMirrorY;
        updateMatrix();
    }
    
private:
    inline void updateMatrix()
    {
        float dx, dy;
        Math::toVector< Math::Angle< 8 >, float >( m_angle, dx, dy );
        //with angle of zero   get dx=1  dy=0
        //with angle of pi/2   get dx=0  dy=1
        //with angle of pi     get dx=-1 dy=0
        //with angle of 1.5pi  get dx=0  dy=-1
        
        const float mx = m_bMirrorX ? -1.0f : 1.0f, 
                    my = m_bMirrorY ? -1.0f : 1.0f;
        
        m11 = dx * mx;      m21 = -dy * mx;
        m12 = dy * my;      m22 =  dx * my;
    }
    
    Math::Angle< 8 >::Value m_angle = Math::Angle< 8 >::eEast;
    bool m_bMirrorX = false, m_bMirrorY = false;
};
    
} 

namespace Ed
{
    using namespace std::string_literals;
    static const std::string szAngles[] = 
    {
        "East"s,
        "NorthEast"s,
        "North"s,
        "NorthWest"s,
        "West"s,
        "SouthWest"s,
        "South"s,
        "SouthEast"s
    };
        
    inline OShorthandStream& operator<<( OShorthandStream& os, const Blueprint::Transform& v )
    {
        std::ostringstream osAngle;
        {
            osAngle << szAngles[ v.Angle() ] << ';';
        }
        return os << v.X() << v.Y() << osAngle.str() << v.MirrorX() << v.MirrorY();
    }

    inline IShorthandStream& operator>>( IShorthandStream& is, Blueprint::Transform& v )
    {
        float x, y;
        Math::Angle< 8 >::Value angle;
        bool bMirrorX, bMirrorY;
        
        {
            std::string strAngle;
            is >> x >> y >> strAngle >> bMirrorX >> bMirrorY;  

            if( strAngle.empty() || strAngle.back() != ';' )
                throw std::runtime_error( "Invalid angle" );
            strAngle.pop_back();

            {
                const std::string* pFind =
                    std::find( szAngles, szAngles + 8, strAngle );
                const std::size_t szDistance = std::distance( szAngles, pFind );
                if( szDistance >= 8 )
                    throw std::runtime_error( "Invalid angle" );
                
                angle = static_cast< Math::Angle< 8 >::Value >( szDistance );
            }
        }
        
        v = Blueprint::Transform( x, y, angle, bMirrorX, bMirrorY );
        
        return is;
    }
}

#endif //TRANSFORM_07_NOV_2020