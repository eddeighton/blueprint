

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
    
    static const float tol = 0.001f;
    
    Blueprint::Transform t1( 3.0f, 4.0f );
    
    ASSERT_NEAR( t1.X(),   3.0f, tol );
    ASSERT_NEAR( t1.Y(),   4.0f, tol );
    ASSERT_NEAR( t1.M11(), 1.0f, tol );
    ASSERT_NEAR( t1.M21(), 0.0f, tol );
    ASSERT_NEAR( t1.M12(), 0.0f, tol );
    ASSERT_NEAR( t1.M22(), 1.0f, tol );
    
    Blueprint::Matrix m1 = Blueprint::Transform::flipVertically( t1, 6.0f, 5.0f, 10.0f, 9.0f  );
    
    ASSERT_NEAR( m1.X(),   3.0f  , tol );
    ASSERT_NEAR( m1.Y(),   10.0f , tol );
    ASSERT_NEAR( m1.M11(), 1.0f  , tol );
    ASSERT_NEAR( m1.M21(), 0.0f  , tol );
    ASSERT_NEAR( m1.M12(), 0.0f  , tol );
    ASSERT_NEAR( m1.M22(), -1.0f , tol );
    
    Blueprint::Matrix m2 = Blueprint::Transform::flipVertically( m1, 6.0f, 5.0f, 10.0f, 9.0f );
    
    ASSERT_NEAR( m2.X(),   3.0f , tol );
    ASSERT_NEAR( m2.Y(),   4.0f , tol );
    ASSERT_NEAR( m2.M11(), 1.0f , tol );
    ASSERT_NEAR( m2.M21(), 0.0f , tol );
    ASSERT_NEAR( m2.M12(), 0.0f , tol );
    ASSERT_NEAR( m2.M22(), 1.0f , tol );
}


using MatrixSet = std::vector< Blueprint::Matrix >;
void constructTransformSet( const Blueprint::Point2D& centre, MatrixSet& transformSet, MatrixSet& inverseTransformSet  )
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

std::vector< std::pair< float, float > > points = 
{
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 0.0f, 1.0f },
    { 1.0f, 1.0f },
    { -1.0f, 0.0f },
    { -1.0f, -1.0f },
    { 1.0f, -1.0f },
    { -1.0f, 1.0f },
    { 123.0f, 321.0f },
    { 321.0f, 123.0f },
    { 321.0f, -123.0f },
    { -321.0f, 123.0f },
    { -321.0f, -123.0f }
};

TEST( Transform, Inverse )
{
    static const float tol = 0.001f;
    
    const Blueprint::Point2D centre = wykobi::make_point< float >( 2.0f, 3.0f );
    
    Blueprint::Matrix transform =
        Blueprint::Transform::inverse( centre.x,  centre.y,  Math::Angle< 8 >::eNorth,     true );
        
    Blueprint::Matrix inverse =
        Blueprint::Transform( centre.x, centre.y, Math::Angle< 8 >::eNorth,            true, false );
        
    
    const Blueprint::Point2D pt = wykobi::make_point< float >( 1.0f, 2.0f );
    
    Blueprint::Point2D ptCopy = pt;
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
    
    static const float tol = 0.001f;
    
    for( const std::pair< float, float >& centrePoint : points )
    {
        const Blueprint::Point2D centre = wykobi::make_point< float >( centrePoint.first, centrePoint.second );
        
        MatrixSet transforms, inverses;
        constructTransformSet( centre, transforms, inverses );
        
        for( const std::pair< float, float >& p : points )
        {
            const Blueprint::Point2D pt = wykobi::make_point< float >( p.first, p.second );
            
            for( std::size_t sz = 0U; sz != inverses.size(); ++sz )
            {
                const Blueprint::Matrix& transform  = transforms[ sz ];
                const Blueprint::Matrix& inverse    = inverses[ sz ];
                
                Blueprint::Point2D ptCopy = pt;
                transform.transform( ptCopy.x, ptCopy.y );
                inverse.transform( ptCopy.x, ptCopy.y );
                
                ASSERT_NEAR( ptCopy.x, pt.x, tol );
                ASSERT_NEAR( ptCopy.y, pt.y, tol );
            }
        }
    }
}