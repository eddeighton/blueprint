
#ifndef TRANSFORM_07_NOV_2020
#define TRANSFORM_07_NOV_2020

#include "blueprint/cgalSettings.h"

#include "common/angle.hpp"

#include "ed/nodeio.hpp"

#include <string>
#include <sstream>

namespace Blueprint
{
    
    inline Transform translate( const Vector v )
    {
        return Transform( CGAL::TRANSLATION, v );
    }
    
    inline Transform rotate( Math::Angle< 8 >::Value angle )
    {
        //Float dx, dy;
        //Math::toVectorDiscrete< Math::Angle< 8 >, Float >( angle, dx, dy );
        //return Transform( CGAL::ROTATION, Direction( dx, dy ), 1, 100 );
        
        const double a = ( angle * MY_PI * 2.0 ) / Math::Angle< 8 >::TOTAL_ANGLES;
        return Transform( CGAL::ROTATION, sin( a ), cos( a ) );
    }
    
    inline const Transform& mirrorX()
    {
        static Transform reflectX( CGAL::REFLECTION, Line( Point( 0, 0 ), Point( 1, 0 ) ) );
        return reflectX;
    }
    
    inline const Transform& mirrorY()
    {
        static Transform reflectY( CGAL::REFLECTION, Line( Point( 0, 0 ), Point( 0, 1 ) ) );
        return reflectY;
    }
    
    /*
    inline Transform translateRotateMirror( Float x, Float y, Math::Angle< 8 >::Value angle, bool bMirrorX, bool bMirrorY )
    {        
        Transform result = rotate( angle );
        
        if( bMirrorX )
        {
            result = result * mirrorX();
        }
        if( bMirrorY )
        {
            result = result * mirrorY();
        }
        
        result = result * translate( Vector( x, y ) );
        
        return result;
    }*/
    
    /*
    inline void decompose( const Transform& transform,
        Float& fTranslateX, Float& fTranslateY, Float& scaleX, Float& scaleY, Float& angle )
    {
        //convert to transform from matrix
        //https://stackoverflow.com/questions/45159314/decompose-2d-transformation-matrix
        
        fTranslateX = CGAL::to_double( transform.m( 0, 2 ) );
        fTranslateY = CGAL::to_double( transform.m( 1, 2 ) );

        scaleX = sqrt( CGAL::to_double( ( transform.m( 0, 0 ) * transform.m( 0, 0 ) ) + ( transform.m( 0, 1 ) * transform.m( 0, 1 ) ) ) );
        scaleY = sqrt( CGAL::to_double( ( transform.m( 1, 0 ) * transform.m( 1, 0 ) ) + ( transform.m( 1, 1 ) * transform.m( 1, 1 ) ) ) );
        
        angle = atan2( CGAL::to_double( transform.m( 0, 1 ) ), 
                        CGAL::to_double( transform.m( 0, 0 ) ) );
    }*/
    
    inline Vector getTranslation( const Transform& transform )
    {
        return Vector( transform.m( 0, 2 ), transform.m( 1, 2 ) );
    }
    
    inline void setTranslation( Transform& transform, Float fTranslateX, Float fTranslateY )
    {
        transform = Transform( 
            transform.m( 0, 0 ), transform.m( 0, 1 ), fTranslateX,
            transform.m( 1, 0 ), transform.m( 1, 1 ), fTranslateY );
    }
    
    inline Transform transformWithinBounds( const Transform& existing, const Transform& transform, const Rect& transformBounds )
    {
        Transform result = existing;
        {
            const auto fCentreX = transformBounds.xmin() + ( transformBounds.xmax() - transformBounds.xmin() ) / 2.0f;
            const auto fCentreY = transformBounds.ymin() + ( transformBounds.ymax() - transformBounds.ymin() ) / 2.0f;
            
            const Transform translateToBoundsCentre = translate( Vector( -fCentreX, -fCentreY ) );
            const Transform translateBack           = translate( Vector(  fCentreX,  fCentreY ) );
            
            //pre multiply
            result = translateToBoundsCentre * result;
            result = transform * result;
            result = translateBack * result;
        }
        return result;
    }
    
    static Transform rotateLeft( const Transform& existing, const Rect& transformBounds )
    {
        static const auto leftTurn = Math::rotate< Math::Angle< 8 > >( Math::Angle< 8 >::eEast, 1 );
        return transformWithinBounds( existing, rotate( leftTurn ), transformBounds );
    }
    
    static Transform rotateRight( const Transform& existing, const Rect& transformBounds )
    {
        static const auto rightTurn = Math::rotate< Math::Angle< 8 > >( Math::Angle< 8 >::eEast, -1 );
        return transformWithinBounds( existing, rotate( rightTurn ), transformBounds );
    }
    
    static Transform flipHorizontally( const Transform& existing, const Rect& transformBounds )
    {
        return transformWithinBounds( existing, mirrorY(), transformBounds );
    }
    
    static Transform flipVertically( const Transform& existing, const Rect& transformBounds )
    {
        return transformWithinBounds( existing, mirrorX(), transformBounds );
    }
    
/*
    static Transform inverse( Float x, Float y, Math::Angle< 8 >::Value angle, bool bMirrorX )
    {
        Transform matrix;
                
        {
            const TranslateRotateMirror translation( -x, -y );
            translation.transform( matrix );
        }
        
        if( bMirrorX )
        {
            const TranslateRotateMirror mirror( 0, 0, Math::Angle< 8 >::eEast, true, false );
            mirror.transform( matrix );
        }
            
        if( angle != Math::Angle< 8 >::eEast )
        {
            const TranslateRotateMirror rotate( 0, 0, 
                static_cast< Math::Angle< 8 >::Value >( 8 - ( static_cast< int >( angle ) ) ), 
                false, false );
            rotate.transform( matrix );
        }
        
        return matrix;
    }*/
    /*
    
    /*
    
    static Transform rotateLeft( const Transform& existing, Float fMinX, Float fMinY, Float fMaxX, Float fMaxY )
    {
        const TranslateRotateMirror transform( 0.0f, 0.0f, 
            Math::rotate< Math::Angle< 8 > >( Math::Angle< 8 >::eEast, 1 ), 
            false, false );
        return transformWithinBounds( existing, transform, fMinX, fMinY, fMaxX, fMaxY );
    }
    
    static Transform rotateRight( const Transform& existing, Float fMinX, Float fMinY, Float fMaxX, Float fMaxY )
    {
        const TranslateRotateMirror transform( 0.0f, 0.0f, 
            Math::rotate< Math::Angle< 8 > >( Math::Angle< 8 >::eEast, -1 ), 
            false, false );
        return transformWithinBounds( existing, transform, fMinX, fMinY, fMaxX, fMaxY );
    }
    
    static Transform flipHorizontally( const Transform& existing, Float fMinX, Float fMinY, Float fMaxX, Float fMaxY )
    {
        const TranslateRotateMirror transform( 0.0f, 0.0f, Math::Angle< 8 >::eEast, true, false );
        return transformWithinBounds( existing, transform, fMinX, fMinY, fMaxX, fMaxY );
    }
    
    static Transform flipVertically( const Transform& existing, Float fMinX, Float fMinY, Float fMaxX, Float fMaxY )
    {
        const TranslateRotateMirror transform( 0.0f, 0.0f, Math::Angle< 8 >::eEast, false, true );
        return transformWithinBounds( existing, transform, fMinX, fMinY, fMaxX, fMaxY );
    }
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
    }*/
   
} 

namespace Ed
{
    inline OShorthandStream& operator<<( OShorthandStream& os, const Blueprint::Transform& v )
    {
        os <<  CGAL::to_double( v.m( 0, 0 ) )
            << CGAL::to_double( v.m( 1, 0 ) )
            << CGAL::to_double( v.m( 0, 1 ) )
            << CGAL::to_double( v.m( 1, 1 ) )
            << CGAL::to_double( v.m( 0, 2 ) )
            << CGAL::to_double( v.m( 1, 2 ) );
        
        return os;
    }

    inline IShorthandStream& operator>>( IShorthandStream& is, Blueprint::Transform& v )
    {
        double m00, m01, m02, m10, m11, m12;
        
        is  >> m00
            >> m10
            >> m01
            >> m11
            >> m02
            >> m12;
            
        v = Blueprint::Transform( m00, m01, m02, m10, m11, m12 );
        
        return is;
    }
    
    /*
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
    }*/
}

#endif //TRANSFORM_07_NOV_2020