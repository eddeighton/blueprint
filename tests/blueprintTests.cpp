

//#include "common/edsUnitTestWrapper.h"

#include "ed/file.hpp"

#include "blueprint/factory.h"
#include "blueprint/toolbox.h"
#include "blueprint/site.h"
#include "blueprint/cgalSettings.h"
#include "blueprint/compiler.h"

#include "blueprint/serialisation.h"

#include "common/file.hpp"

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

/*
TEST( BlueprintTests, Check )
{
    const std::string strFile1 = "../testdata/test1.blu";
    const std::string strFile2 = "../testdata/test1.blu";

    Blueprint::Factory factory;
    Blueprint::Site::Ptr pTest = factory.load( strFile1 );
    ASSERT_TRUE( pTest );

    factory.save( pTest, strFile2 );

    boost::filesystem::path path1 = boost::filesystem::canonical(
        boost::filesystem::absolute( strFile1 ) );
    ASSERT_TRUE( boost::filesystem::exists( path1 ) );

    boost::filesystem::path path2 = boost::filesystem::canonical(
        boost::filesystem::absolute( strFile2 ) );
    ASSERT_TRUE( boost::filesystem::exists( path2 ) );
    
    Ed::BasicFileSystem basicFS;
    Ed::File e1( basicFS, path1.string() ), e2( basicFS, path2.string() );
    
    ASSERT_EQ( e1, e2 );

}*/

TEST( Serialisation, Points )
{
    using namespace Ed;
    wykobi::point2d< float > p = wykobi::make_point< float >( 123.567f, 321.765f );
    std::ostringstream os;
    os << p;

    std::istringstream is( os.str() );

    wykobi::point2d< float > result;
    is >> result;
    ASSERT_NEAR( p.x, result.x, 0.001f );
    ASSERT_NEAR( p.y, result.y, 0.001f );
}

TEST( Serialisation, Polygons )
{
    using namespace Ed;
    wykobi::polygon< float, 2 > p = wykobi::make_polygon< float >( 
        wykobi::make_circle< float >( 0.0f, 0.0f, 123.456f)  );
    std::ostringstream os;
    os << p;

    std::string strResult = os.str();
    std::istringstream is( strResult );

    wykobi::polygon< float, 2 > result;
    is >> result;
    ASSERT_EQ( p.size(), result.size() );
	for ( wykobi::polygon< float, 2 >::const_iterator 
        i = p.begin(), iEnd = p.end(), j = result.begin(); i!=iEnd; ++i,++j)
    {
        ASSERT_NEAR( i->x, j->x, 0.001f );
        ASSERT_NEAR( i->y, j->y, 0.001f );
    }
}
/*
TEST( Toolbox, Check1 )
{
    const char* pszToolboxPath = getenv( "BLUEPRINT_TOOLBOX_PATH" );
    Blueprint::Toolbox::Ptr pToolbox( new Blueprint::Toolbox( pszToolboxPath ) );
    ASSERT_TRUE( pToolbox->getPalette( "shapes" ) );
}*/

TEST( CGAL, ErrorTest )
{
    using namespace Blueprint;
    {
        const Point_2   p1( 0, 0 ), 
                        p2( sqrt( 2 ), sqrt( 2 ) ), 
                        p3( 2, 2 );
        
        ASSERT_EQ( CGAL::orientation( p1, p2, p3 ), CGAL::COLLINEAR );
    }
    {
        const Point_2   p1( 0, 0 ), 
                        p2( sqrt( 2 ), sqrt( 2 ) ), 
                        p3( 2, 2.0000001f );
        
        ASSERT_EQ( CGAL::orientation( p1, p2, p3 ), CGAL::COLLINEAR );
    }
    {
        const Point_2   p1( 0, 0 ), 
                        p2( sqrt( 2 ), sqrt( 2 ) ), 
                        p3( 2, 2.000001f );
        
        ASSERT_EQ( CGAL::orientation( p1, p2, p3 ), CGAL::LEFT_TURN );
    }
}


TEST( CGAL, BasicTest )
{
    using namespace Blueprint;
    
    Arr_with_hist_2 arr;
    
    Point_2 p1(0, 0), 
            p2(0, 4);
    
    Segment_2 s1( p1, p2 );
    
    Curve_handle ch = CGAL::insert( arr, s1 );
    
    ASSERT_EQ( arr.number_of_curves(), 1 );
}

namespace
{
    using PointVector = std::vector< Blueprint::Point_2 >;
    
    std::vector< Blueprint::Curve_handle > insertPolygon( 
        Blueprint::Arr_with_hist_2& arr, const PointVector& polygon )
    {
        std::vector< Blueprint::Curve_handle > interiorCurves;
        for( PointVector::const_iterator 
            i       = polygon.begin(),
            iNext   = polygon.begin(),
            iEnd    = polygon.end(); i!=iEnd; ++i )
        {
            ++iNext;
            if( iNext == iEnd )
                iNext = polygon.begin();
            
            interiorCurves.push_back( 
                CGAL::insert( arr, Blueprint::Segment_2( *i, *iNext ) ) );
        }
        return interiorCurves;
    }
}

TEST( CGAL, PolygonTest )
{
    using namespace Blueprint;
    
    Arr_with_hist_2 arr;
    
    const PointVector interior = 
    { 
        Point_2{ 0, 0 }, 
        Point_2{ 0, 10 }, 
        Point_2{ 10, 10 }, 
        Point_2{ 10, 0 } 
    };
    
    std::vector< Curve_handle > interiorCurves =
        insertPolygon( arr, interior );
    
    ASSERT_EQ( arr.number_of_curves(), 4 );
    ASSERT_EQ( arr.number_of_halfedges(), 8 );
    ASSERT_EQ( arr.number_of_vertices(), 4 );
    ASSERT_EQ( arr.number_of_faces(), 2 );
}

TEST( CGAL, OverlapTest )
{
    using namespace Blueprint;
    
    Arr_with_hist_2 arr;
    
    const PointVector interior = 
    { 
        Point_2{  0,  0 }, 
        Point_2{  0, 10 }, 
        Point_2{ 10, 10 }, 
        Point_2{ 10,  0 } 
    };
    //       +----+----+
    //       |         |
    //       |         |
    //       |         |
    //       +----+----+
    
    std::vector< Curve_handle > interiorCurves =
        insertPolygon( arr, interior );
        
    const PointVector exterior = 
    { 
        Point_2{  5,  0 }, 
        Point_2{  5, 10 }, 
        Point_2{ 10, 10 }, 
        Point_2{ 10,  0 } 
    };
    //            +----+
    //            |    |
    //            |    |
    //            |    |
    //            +----+
    
    std::vector< Curve_handle > exteriorCurves =
        insertPolygon( arr, exterior );
    
    //       +----+----+
    //       |    |    |
    //       |    |    |
    //       |    |    |
    //       +----+----+
    
    ASSERT_EQ( arr.number_of_curves(), 8 );
    ASSERT_EQ( arr.number_of_halfedges(), 14 );
    ASSERT_EQ( arr.number_of_vertices(), 6 );
    ASSERT_EQ( arr.number_of_faces(), 3 );
    
}


TEST( CGAL, DataTest )
{
    using namespace Blueprint;
    
    Arr_with_hist_2 arr;
    
    const PointVector interior = 
    { 
        Point_2{  0,  0 }, 
        Point_2{  0, 10 }, 
        Point_2{ 20, 10 }, 
        Point_2{ 20,  0 } 
    };
    
    std::vector< Curve_handle > interiorCurves =
        insertPolygon( arr, interior );
        
    const PointVector exterior = 
    { 
        Point_2{  5,  0 }, 
        Point_2{  5, 10 }, 
        Point_2{ 10, 10 }, 
        Point_2{ 10,  0 } 
    };
    
    std::vector< Curve_handle > exteriorCurves =
        insertPolygon( arr, exterior );
    
    {
        for( Arr_with_hist_2::Halfedge_iterator 
            i = arr.halfedges_begin(),
            iEnd = arr.halfedges_end();
            i!=iEnd; ++i )
        {
            i->set_data( 0 );
            //i->twin()->set_data( 0 );
        }
        
        for( Curve_handle ch : interiorCurves )
        {
            for( Arr_with_hist_2::Induced_edge_iterator 
                i       = arr.induced_edges_begin( ch ),
                iEnd    = arr.induced_edges_end( ch );
                i!=iEnd; ++i )
            {
                int iData = (*i)->data();
                (*i)->set_data( iData + 1 );
            }
        }
        
        for( Curve_handle ch : exteriorCurves )
        {
            for( Arr_with_hist_2::Induced_edge_iterator 
                i       = arr.induced_edges_begin( ch ),
                iEnd    = arr.induced_edges_end( ch );
                i!=iEnd; ++i )
            {
                int iData = (*i)->data();
                (*i)->set_data( iData + 1 );
            }
        }
    }
    
    int iCountOne = 0;
    int iCountTwo = 0;
    {
        for( Arr_with_hist_2::Halfedge_iterator 
            i = arr.halfedges_begin(),
            iEnd = arr.halfedges_end();
            i!=iEnd; ++i )
        {
            switch( i->data() )
            {
                case 1: ++iCountOne; break;
                case 2: ++iCountTwo; break;
                default: break;
                    //ASSERT_TRUE( false );
            }
        }
    }
    //       +----+----+----+
    //       |    |    |    |
    //       |    |    |    |
    //       |    |    |    |
    //       +----+----+----+
    ASSERT_EQ( iCountOne, 8 );
    ASSERT_EQ( iCountTwo, 2 );
}

void loadTest( const char* pszFileName )
{
    const boost::filesystem::path installPath = getenv( "BLUEPRINT" );
    boost::filesystem::path testFilePath = installPath / "testfiles" / pszFileName;
    ASSERT_TRUE( boost::filesystem::exists( testFilePath ) );

    Blueprint::Factory factory;
    Blueprint::Site::Ptr pTest = factory.load( testFilePath.string() );
    ASSERT_TRUE( pTest );
    
    Blueprint::Compiler compiler( pTest->getSpaces() );
    
    compiler.generateHTML( testFilePath.replace_extension( ".html" ) );
}

TEST( CGAL, CompilerBasic )
{
    loadTest( "basic.blu" );
}

TEST( CGAL, CompilerBasic_child )
{
    loadTest( "basic_child.blu" );
}

TEST( CGAL, CompilerBasic_connection )
{
    loadTest( "basic_connection.blu" );
}

TEST( CGAL, CompilerBasic_loop )
{
    loadTest( "basic_loop.blu" );
}

TEST( CGAL, CompilerBasic_exterior_connection )
{
    //{
    //    char c;
    //    std::cin >> c;
    //}
    std::cout << "Hello from CompilerBasic_exterior_connection" << std::endl;
    loadTest( "basic_exterior_connection.blu" );
}

TEST( CGAL, CompilerComplex )
{
    loadTest( "complex.blu" );
}

TEST( CGAL, CompilerComplex_big )
{
    loadTest( "complex_big.blu" );
}

