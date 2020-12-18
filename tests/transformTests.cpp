

#include "blueprint/transform.h"
#include "blueprint/geometry.h"

#include <gtest/gtest.h>
#include <iostream>

TEST( Transform, Bounds )
{
    /*{
        char c;
        std::cin >> c;
    }*/
    
    static const Blueprint::Float tol = 0.001;
    
    Blueprint::Transform t1( 3.0, 4.0 );
    
    ASSERT_NEAR( t1.X(),   3.0, tol );
    ASSERT_NEAR( t1.Y(),   4.0, tol );
    ASSERT_NEAR( t1.M11(), 1.0, tol );
    ASSERT_NEAR( t1.M21(), 0.0, tol );
    ASSERT_NEAR( t1.M12(), 0.0, tol );
    ASSERT_NEAR( t1.M22(), 1.0, tol );
    
    Blueprint::Matrix m1 = Blueprint::Transform::flipVertically( t1, 6.0, 5.0, 10.0, 9.0  );
    
    ASSERT_NEAR( m1.X(),   3.0  , tol );
    ASSERT_NEAR( m1.Y(),   10.0 , tol );
    ASSERT_NEAR( m1.M11(), 1.0  , tol );
    ASSERT_NEAR( m1.M21(), 0.0  , tol );
    ASSERT_NEAR( m1.M12(), 0.0  , tol );
    ASSERT_NEAR( m1.M22(), -1.0 , tol );
    
    Blueprint::Matrix m2 = Blueprint::Transform::flipVertically( m1, 6.0, 5.0, 10.0, 9.0 );
    
    ASSERT_NEAR( m2.X(),   3.0 , tol );
    ASSERT_NEAR( m2.Y(),   4.0 , tol );
    ASSERT_NEAR( m2.M11(), 1.0 , tol );
    ASSERT_NEAR( m2.M21(), 0.0 , tol );
    ASSERT_NEAR( m2.M12(), 0.0 , tol );
    ASSERT_NEAR( m2.M22(), 1.0 , tol );
}


using MatrixSet = std::vector< Blueprint::Matrix >;
void constructTransformSet( const wykobi::point2d< Blueprint::Float >& centre, MatrixSet& transformSet, MatrixSet& inverseTransformSet  )
{
    transformSet.push_back( Blueprint::Transform::inverse( centre.x,  centre.y,  Math::Angle< 8 >::eEast,     false ) );
    transformSet.push_back( Blueprint::Transform::inverse( centre.x,  centre.y,  Math::Angle< 8 >::eNorth,    false ) );
    transformSet.push_back( Blueprint::Transform::inverse( centre.x,  centre.y,  Math::Angle< 8 >::eWest,     false ) );
    transformSet.push_back( Blueprint::Transform::inverse( centre.x,  centre.y,  Math::Angle< 8 >::eSouth,    false ) );
    transformSet.push_back( Blueprint::Transform::inverse( centre.x,  centre.y,  Math::Angle< 8 >::eEast,     true ) );
    transformSet.push_back( Blueprint::Transform::inverse( centre.x,  centre.y,  Math::Angle< 8 >::eNorth,    true ) );
    transformSet.push_back( Blueprint::Transform::inverse( centre.x,  centre.y,  Math::Angle< 8 >::eWest,     true ) );
    transformSet.push_back( Blueprint::Transform::inverse( centre.x,  centre.y,  Math::Angle< 8 >::eSouth,    true ) );
    
    inverseTransformSet.push_back( Blueprint::Transform( centre.x, centre.y, Math::Angle< 8 >::eEast,     false, false ) );
    inverseTransformSet.push_back( Blueprint::Transform( centre.x, centre.y, Math::Angle< 8 >::eNorth,    false, false ) );
    inverseTransformSet.push_back( Blueprint::Transform( centre.x, centre.y, Math::Angle< 8 >::eWest,     false, false ) );
    inverseTransformSet.push_back( Blueprint::Transform( centre.x, centre.y, Math::Angle< 8 >::eSouth,    false, false ) );
    inverseTransformSet.push_back( Blueprint::Transform( centre.x, centre.y, Math::Angle< 8 >::eEast,     true,  false ) );
    inverseTransformSet.push_back( Blueprint::Transform( centre.x, centre.y, Math::Angle< 8 >::eNorth,    true,  false ) );
    inverseTransformSet.push_back( Blueprint::Transform( centre.x, centre.y, Math::Angle< 8 >::eWest,     true,  false ) );
    inverseTransformSet.push_back( Blueprint::Transform( centre.x, centre.y, Math::Angle< 8 >::eSouth,    true,  false ) );
}

std::vector< std::pair< Blueprint::Float, Blueprint::Float > > points = 
{
    { 0.0, 0.0 },
    { 1.0, 0.0 },
    { 0.0, 1.0 },
    { 1.0, 1.0 },
    { -1.0, 0.0 },
    { -1.0, -1.0 },
    { 1.0, -1.0 },
    { -1.0, 1.0 },
    { 123.0, 321.0 },
    { 321.0, 123.0 },
    { 321.0, -123.0 },
    { -321.0, 123.0 },
    { -321.0, -123.0 }
};

TEST( Transform, Inverse )
{
    static const Blueprint::Float tol = 0.001;
    
    const wykobi::point2d< Blueprint::Float > centre = wykobi::make_point< Blueprint::Float >( 2.0, 3.0 );
    
    Blueprint::Matrix transform =
        Blueprint::Transform::inverse( centre.x,  centre.y,  Math::Angle< 8 >::eNorth,     true );
        
    Blueprint::Matrix inverse =
        Blueprint::Transform( centre.x, centre.y, Math::Angle< 8 >::eNorth,            true, false );
        
    const wykobi::point2d< Blueprint::Float > pt = wykobi::make_point< Blueprint::Float >( 1.0, 2.0 );
    
    wykobi::point2d< Blueprint::Float > ptCopy = pt;
    transform.transform( ptCopy.x, ptCopy.y );
    inverse.transform( ptCopy.x, ptCopy.y );
    
    ASSERT_NEAR( ptCopy.x, pt.x, tol );
    ASSERT_NEAR( ptCopy.y, pt.y, tol );
}

TEST( Transform, Inverse_Brute )
{
    /*{
        char c;
        std::cin >> c;
    }*/
    
    static const Blueprint::Float tol = 0.001;
    
    for( const std::pair< Blueprint::Float, Blueprint::Float >& centrePoint : points )
    {
        const wykobi::point2d< Blueprint::Float > centre = wykobi::make_point< Blueprint::Float >( centrePoint.first, centrePoint.second );
        
        MatrixSet transforms, inverses;
        constructTransformSet( centre, transforms, inverses );
        
        for( const std::pair< Blueprint::Float, Blueprint::Float >& p : points )
        {
            const wykobi::point2d< Blueprint::Float > pt = wykobi::make_point< Blueprint::Float >( p.first, p.second );
            
            for( std::size_t sz = 0U; sz != inverses.size(); ++sz )
            {
                const Blueprint::Matrix& transform  = transforms[ sz ];
                const Blueprint::Matrix& inverse    = inverses[ sz ];
                
                wykobi::point2d< Blueprint::Float > ptCopy = pt;
                transform.transform( ptCopy.x, ptCopy.y );
                inverse.transform( ptCopy.x, ptCopy.y );
                
                ASSERT_NEAR( ptCopy.x, pt.x, tol );
                ASSERT_NEAR( ptCopy.y, pt.y, tol );
            }
        }
    }
}

TEST( Transform, Decompose )
{
    static const Blueprint::Float tol = 0.001f;
    
    for( bool bMirrorX = false; !bMirrorX; bMirrorX = !bMirrorX )
    {
        for( bool bMirrorY = false; !bMirrorY; bMirrorY = !bMirrorY )
        {
            for( std::size_t sz = 0U; sz != 8; ++sz )
            {
                const Math::Angle< 8 >::Value angle = 
                    static_cast< Math::Angle< 8 >::Value >( sz );
                
                for( const std::pair< Blueprint::Float, Blueprint::Float >& p : points )
                {
                    const Blueprint::Transform transform( 
                        p.first, 
                        p.second, 
                        static_cast< Math::Angle< 8 >::Value >( sz ), 
                        bMirrorX, 
                        bMirrorY );
                    
                    Blueprint::Float fTranslateX;
                    Blueprint::Float fTranslateY;
                    Blueprint::Float fScaleX;
                    Blueprint::Float fScaleY;
                    Blueprint::Float fAngle;
                    transform.decompose( fTranslateX, fTranslateY, fScaleX, fScaleY, fAngle );
                    
                    ASSERT_NEAR( fTranslateX, p.first, tol );
                    ASSERT_NEAR( fTranslateY, p.second, tol );
                    
                    if( bMirrorX )
                    {
                        ASSERT_NEAR( fScaleX, -1.0f, tol );
                    }
                    else
                    {
                        ASSERT_NEAR( fScaleX, 1.0f, tol );
                    }
                    if( bMirrorY )
                    {
                        ASSERT_NEAR( fScaleY, -1.0f, tol );
                    }
                    else
                    {
                        ASSERT_NEAR( fScaleY, 1.0f, tol );
                    }
                    
                    while( fAngle < 0.0f )
                        fAngle += MY_PI * 2.0f;
                    
                    ASSERT_NEAR( fAngle, Math::toRadians< Math::Angle< 8 > >( angle ), tol );
                    
                }
                
            }
        }
    }
    
}