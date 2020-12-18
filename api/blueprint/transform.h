
#ifndef TRANSFORM_07_NOV_2020
#define TRANSFORM_07_NOV_2020

#include "blueprint/cgalSettings.h"

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
    Matrix( Float x, Float y )
        :   m31( x ),
            m32( y )
    {}
    
    inline Float M11() const { return m11; }
    inline Float M12() const { return m12; }
    inline Float M21() const { return m21; }
    inline Float M22() const { return m22; }
    inline Float M31() const { return m31; }
    inline Float M32() const { return m32; }
    inline Float X()   const { return m31; }
    inline Float Y()   const { return m32; }
    
    void assign( const Matrix& a )
    {
        m11 = a.m11 ;
        m21 = a.m21 ;
        m31 = a.m31 ;
        m12 = a.m12 ;
        m22 = a.m22 ;
        m32 = a.m32 ;
    }
    
    inline void transform( Float& x, Float& y ) const
    {
        Float xTemp = x;
        Float yTemp = y;
        
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
    
    void setTranslation( Float x, Float y )
    {
        m31 = x;
        m32 = y;
    }
    
    void translateBy( Float x, Float y )
    {
        m31 += x;
        m32 += y;
    }
    
    void decompose( Float& fTranslateX, Float& fTranslateY, Float& scaleX, Float& scaleY, Float& angle ) const
    {
        //convert to transform from matrix
        //https://stackoverflow.com/questions/45159314/decompose-2d-transformation-matrix
        
        fTranslateX = m31;
        fTranslateY = m32;

        scaleX = sqrt( ( m11 * m11 ) + ( m21 * m21 ) );
        scaleY = sqrt( ( m12 * m12 ) + ( m22 * m22 ) );
        
        angle = atan2( m21, m11 );
    }
    
    //  m(x,y) for row,column
    //                col 0       col 1       col 2
    Float /*row 0*/   m11 = 1.0f, m21= 0.0f,  m31 = 0.0f, 
          /*row 1*/   m12 = 0.0f, m22 = 1.0f, m32 = 0.0f;
          /*row 2*/// m13 = 0     m23 - 0     m33 = 1
};

class Transform : public Matrix
{
public:
    Transform()
    {
    }
    
    Transform( Float x, Float y )
        :   Matrix( x, y ),
            m_angle( Math::Angle< 8 >::eEast ),
            m_bMirrorX( false ),
            m_bMirrorY( false )
    {}
    
    Transform( Float x, Float y, Math::Angle< 8 >::Value angle, bool bMirrorX, bool bMirrorY )
        :   Matrix( x, y ),
            m_angle( angle ),
            m_bMirrorX( bMirrorX ),
            m_bMirrorY( bMirrorY )
    {
        updateMatrix();
    }
    
    static Matrix inverse( Float x, Float y, Math::Angle< 8 >::Value angle, bool bMirrorX )
    {
        Matrix matrix;
                
        {
            const Transform translation( -x, -y );
            translation.transform( matrix );
        }
        
        if( bMirrorX )
        {
            const Transform mirror( 0, 0, Math::Angle< 8 >::eEast, true, false );
            mirror.transform( matrix );
        }
            
        if( angle != Math::Angle< 8 >::eEast )
        {
            const Transform rotate( 0, 0, 
                static_cast< Math::Angle< 8 >::Value >( 8 - ( static_cast< int >( angle ) ) ), 
                false, false );
            rotate.transform( matrix );
        }
        
        return matrix;
    }
    
    inline Math::Angle< 8 >::Value Angle() const { return m_angle; }
    inline bool MirrorX() const { return m_bMirrorX; }
    inline bool MirrorY() const { return m_bMirrorY; }
    
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
    
    static Matrix transformWithinBounds( const Matrix& existing, const Matrix& transform, Float fMinX, Float fMinY, Float fMaxX, Float fMaxY )
    {
        Matrix result;
        {
            const Float fCentreX = fMinX + ( fMaxX - fMinX ) / 2.0f;
            const Float fCentreY = fMinY + ( fMaxY - fMinY ) / 2.0f;
            
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
    
    static Matrix rotateLeft( const Matrix& existing, Float fMinX, Float fMinY, Float fMaxX, Float fMaxY )
    {
        const Transform transform( 0.0f, 0.0f, 
            Math::rotate< Math::Angle< 8 > >( Math::Angle< 8 >::eEast, 1 ), 
            false, false );
        return transformWithinBounds( existing, transform, fMinX, fMinY, fMaxX, fMaxY );
    }
    
    static Matrix rotateRight( const Matrix& existing, Float fMinX, Float fMinY, Float fMaxX, Float fMaxY )
    {
        const Transform transform( 0.0f, 0.0f, 
            Math::rotate< Math::Angle< 8 > >( Math::Angle< 8 >::eEast, -1 ), 
            false, false );
        return transformWithinBounds( existing, transform, fMinX, fMinY, fMaxX, fMaxY );
    }
    
    static Matrix flipHorizontally( const Matrix& existing, Float fMinX, Float fMinY, Float fMaxX, Float fMaxY )
    {
        const Transform transform( 0.0f, 0.0f, Math::Angle< 8 >::eEast, true, false );
        return transformWithinBounds( existing, transform, fMinX, fMinY, fMaxX, fMaxY );
    }
    
    static Matrix flipVertically( const Matrix& existing, Float fMinX, Float fMinY, Float fMaxX, Float fMaxY )
    {
        const Transform transform( 0.0f, 0.0f, Math::Angle< 8 >::eEast, false, true );
        return transformWithinBounds( existing, transform, fMinX, fMinY, fMaxX, fMaxY );
    }
    
private:
    inline void updateMatrix()
    {
        Float dx, dy;
        Math::toVector< Math::Angle< 8 >, Float >( m_angle, dx, dy );
        //with angle of zero   get dx=1  dy=0
        //with angle of pi/2   get dx=0  dy=1
        //with angle of pi     get dx=-1 dy=0
        //with angle of 1.5pi  get dx=0  dy=-1
        
        const Float mx = m_bMirrorX ? -1.0f : 1.0f, 
                    my = m_bMirrorY ? -1.0f : 1.0f;
        
        m11 = dx * mx;      m21 =  dy * mx;
        m12 = -dy * my;     m22 =  dx * my;
    }
    
    Math::Angle< 8 >::Value m_angle = Math::Angle< 8 >::eEast;
    bool m_bMirrorX = false, m_bMirrorY = false;
};
    
} 

namespace Ed
{
    inline OShorthandStream& operator<<( OShorthandStream& os, const Blueprint::Matrix& v )
    {
        os << v.m11
            << v.m21
            << v.m31
            << v.m12
            << v.m22
            << v.m32;
        
        return os;
    }

    inline IShorthandStream& operator>>( IShorthandStream& is, Blueprint::Matrix& v )
    {
        is >> v.m11
            >> v.m21
            >> v.m31
            >> v.m12
            >> v.m22
            >> v.m32;
        
        return is;
    }
    
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
        Blueprint::Float x, y;
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