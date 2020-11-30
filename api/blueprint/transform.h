
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
        :   m31( x ),
            m32( y )
    {}
    
    inline float M11() const { return m11; }
    inline float M12() const { return m12; }
    inline float M21() const { return m21; }
    inline float M22() const { return m22; }
    inline float M31() const { return m31; }
    inline float M32() const { return m32; }
    inline float X()   const { return m31; }
    inline float Y()   const { return m32; }
    
    void assign( const Matrix& a )
    {
        m11 = a.m11 ;
        m21 = a.m21 ;
        m31 = a.m31 ;
        m12 = a.m12 ;
        m22 = a.m22 ;
        m32 = a.m32 ;
    }
    
    inline void transform( float& x, float& y ) const
    {
        float xTemp = x;
        float yTemp = y;
        
        x = ( m11 * xTemp ) + ( m21 * yTemp ) + m31;
        y = ( m12 * xTemp ) + ( m22 * yTemp ) + m32;
    }
    
    inline void transform( Matrix& matrix ) const
    {
        const Matrix temp = matrix;
        
        matrix.m11 = ( m11 * temp.m11 ) + ( m21 * temp.m12 );// + ( m31 * temp.m13 );
        matrix.m21 = ( m11 * temp.m21 ) + ( m21 * temp.m22 );// + ( m31 * temp.m23 );
        matrix.m31 = ( m11 * temp.m31 ) + ( m21 * temp.m32 ) + ( m31 /** temp.m33*/ );
        
        matrix.m12 = ( m12 * temp.m11 ) + ( m22 * temp.m12 );// + ( m32 * temp.m13 );
        matrix.m22 = ( m12 * temp.m21 ) + ( m22 * temp.m22 );// + ( m32 * temp.m23 );
        matrix.m32 = ( m12 * temp.m31 ) + ( m22 * temp.m32 ) + ( m32 /** temp.m33*/ );
        
       // matrix.m13 = ( m13 * temp.m11 ) + ( m23 * temp.m12 ) + ( m33 * temp.m13 );
       // matrix.m23 = ( m13 * temp.m21 ) + ( m23 * temp.m22 ) + ( m33 * temp.m23 );
       // matrix.m33 = ( m13 * temp.m31 ) + ( m23 * temp.m32 ) + ( m33 * temp.m33 );
    }
    
protected:

    //  m(x,y) for row,column
    //                col 0       col 1       col 2
    float /*row 0*/   m11 = 1.0f, m21= 0.0f,  m31 = 0.0f, 
          /*row 1*/   m12 = 0.0f, m22 = 1.0f, m32 = 0.0f;
          /*row 2*/// m13 = 0     m23 - 0     m33 = 1
};
    
class Transform : public Matrix
{
public:
    Transform()
    {
    }
    
    Transform( float x, float y )
        :   Matrix( x, y ),
            m_angle( Math::Angle< 8 >::eEast ),
            m_bMirrorX( false ),
            m_bMirrorY( false )
    {}
    
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
        m31 = x;
        m32 = y;
        updateMatrix();
    }
    
    void translateBy( float x, float y )
    {
        m31 += x;
        m32 += y;
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
    
    static Matrix transformWithinBounds( const Matrix& existing, const Matrix& transform, float fMinX, float fMinY, float fMaxX, float fMaxY )
    {
        Matrix result;
        {
            const float fCentreX = fMinX + ( fMaxX - fMinX ) / 2.0f;
            const float fCentreY = fMinY + ( fMaxY - fMinY ) / 2.0f;
            
            const Matrix translateToBoundsCentre( -fCentreX, -fCentreY );
            const Matrix translateBack( fCentreX, fCentreY );
            
            //pre multiply
            existing.transform( result );
            translateToBoundsCentre.transform( result );
            transform.transform( result );
            translateBack.transform( result );
        }
        return result;
    }
    
    void rotateLeft( float fMinX, float fMinY, float fMaxX, float fMaxY )
    {
        m_angle = Math::rotate< Math::Angle< 8 > >( m_angle, 1 );
        
        const Transform transform( 0.0f, 0.0f, 
            Math::rotate< Math::Angle< 8 > >( Math::Angle< 8 >::eEast, 1 ), 
            false, false );
        
        const Matrix matrix = transformWithinBounds( *this, transform, fMinX, fMinY, fMaxX, fMaxY );
        assign( matrix );
    }
    
    void rotateRight( float fMinX, float fMinY, float fMaxX, float fMaxY )
    {
        m_angle = Math::rotate< Math::Angle< 8 > >( m_angle, -1 );
        
        const Transform transform( 0.0f, 0.0f, 
            Math::rotate< Math::Angle< 8 > >( Math::Angle< 8 >::eEast, -1 ), 
            false, false );
            
        const Matrix matrix = transformWithinBounds( *this, transform, fMinX, fMinY, fMaxX, fMaxY );
        assign( matrix );
    }
    
    void flipHorizontally( float fMinX, float fMinY, float fMaxX, float fMaxY )
    {
        m_bMirrorX = !m_bMirrorX;
        const Transform transform( 0.0f, 0.0f, Math::Angle< 8 >::eEast, true, false );
            
        const Matrix matrix = transformWithinBounds( *this, transform, fMinX, fMinY, fMaxX, fMaxY );
        assign( matrix );
    }
    
    void flipVertically( float fMinX, float fMinY, float fMaxX, float fMaxY )
    {
        m_bMirrorY = !m_bMirrorY;
        const Transform transform( 0.0f, 0.0f, Math::Angle< 8 >::eEast, false, true );
        
        const Matrix matrix = transformWithinBounds( *this, transform, fMinX, fMinY, fMaxX, fMaxY );
        assign( matrix );
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