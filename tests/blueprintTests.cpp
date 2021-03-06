

//#include "common/edsUnitTestWrapper.h"

#include "ed/file.hpp"

#include "blueprint/factory.h"
#include "blueprint/toolbox.h"
#include "blueprint/site.h"
#include "blueprint/cgalSettings.h"
#include "blueprint/transform.h"
#include "blueprint/blueprint.h"
#include "blueprint/compilation.h"
#include "blueprint/visibility.h"

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
/*
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
    wykobi::polygon< Float, 2 > p = wykobi::make_polygon< float >( 
        wykobi::make_circle< float >( 0.0f, 0.0f, 123.456f)  );
    std::ostringstream os;
    os << p;

    std::string strResult = os.str();
    std::istringstream is( strResult );

    wykobi::polygon< Float, 2 > result;
    is >> result;
    ASSERT_EQ( p.size(), result.size() );
	for ( wykobi::polygon< Float, 2 >::const_iterator 
        i = p.begin(), iEnd = p.end(), j = result.begin(); i!=iEnd; ++i,++j)
    {
        ASSERT_NEAR( i->x, j->x, 0.001f );
        ASSERT_NEAR( i->y, j->y, 0.001f );
    }
}*/
/*
TEST( Toolbox, Check1 )
{
    const char* pszToolboxPath = getenv( "BLUEPRINT_TOOLBOX_PATH" );
    Blueprint::Toolbox::Ptr pToolbox( new Blueprint::Toolbox( pszToolboxPath ) );
    ASSERT_TRUE( pToolbox->getPalette( "shapes" ) );
}
*/
TEST( CGAL, ErrorTest )
{
    using namespace Blueprint;
    {
        const Point   p1( 0, 0 ), 
                        p2( sqrt( 2 ), sqrt( 2 ) ), 
                        p3( 2, 2 );
        
        ASSERT_EQ( CGAL::orientation( p1, p2, p3 ), CGAL::COLLINEAR );
    }
    {
        const Point   p1( 0, 0 ), 
                        p2( sqrt( 2 ), sqrt( 2 ) ), 
                        p3( 2, 2.0000001f );
        
        ASSERT_EQ( CGAL::orientation( p1, p2, p3 ), CGAL::COLLINEAR );
    }
    {
        const Point   p1( 0, 0 ), 
                        p2( sqrt( 2 ), sqrt( 2 ) ), 
                        p3( 2, 2.000001f );
        
        ASSERT_EQ( CGAL::orientation( p1, p2, p3 ), CGAL::LEFT_TURN );
    }
}


TEST( CGAL, BasicTest )
{
    using namespace Blueprint;
    
    Arrangement arr;
    
    Point p1(0, 0), 
            p2(0, 4);
    
    Curve s1( p1, p2 );
    
    Curve_handle ch = CGAL::insert( arr, s1 );
    
    ASSERT_EQ( arr.number_of_curves(), 1 );
}

namespace
{
    using PointVector = std::vector< Blueprint::Point >;
    
    std::vector< Blueprint::Curve_handle > insertPolygon( 
        Blueprint::Arrangement& arr, const PointVector& polygon )
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
                CGAL::insert( arr, Blueprint::Curve( *i, *iNext ) ) );
        }
        return interiorCurves;
    }
}

TEST( CGAL, PolygonTest )
{
    using namespace Blueprint;
    
    Arrangement arr;
    
    const PointVector interior = 
    { 
        Point{ 0, 0 }, 
        Point{ 0, 10 }, 
        Point{ 10, 10 }, 
        Point{ 10, 0 } 
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
    
    Arrangement arr;
    
    const PointVector interior = 
    { 
        Point{  0,  0 }, 
        Point{  0, 10 }, 
        Point{ 10, 10 }, 
        Point{ 10,  0 } 
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
        Point{  5,  0 }, 
        Point{  5, 10 }, 
        Point{ 10, 10 }, 
        Point{ 10,  0 } 
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

static const boost::filesystem::path testFilesFolderPath = getenv( "BLUEPRINT" );
    
boost::filesystem::path constructPath( const boost::filesystem::path& inputFile, const char* pszExt )
{
    boost::filesystem::path t = inputFile;
    t.replace_extension( "" );
    std::ostringstream os;
    os << t.filename().string() << pszExt;
    return t.parent_path() / os.str();
}

void loadTest( const boost::filesystem::path& inputFile )
{    
    try
    {
        std::cout << "testing: " << inputFile.string() << std::endl;
        Blueprint::Factory factory;
        Blueprint::Blueprint::Ptr pTest = 
            boost::dynamic_pointer_cast< ::Blueprint::Blueprint >( 
                factory.load( inputFile.string() ) );
        ASSERT_TRUE( pTest );
        
        {
            const Blueprint::Site::EvaluationMode mode = { true, false, false };
            Blueprint::Site::EvaluationResults results;
            pTest->evaluate( mode, results );
        }
        
        Blueprint::Compilation compilation( pTest );
        //Blueprint::Compilation::SpacePolyMap spacePolyMap;
        //compilation.getSpacePolyMap( spacePolyMap );
        
        compilation.render( constructPath( inputFile, ".html" ) );
        compilation.renderFillers( constructPath( inputFile, "__fillers.html" ) );
        compilation.renderFloors( constructPath( inputFile, "__floors.html" ) );
        
        Blueprint::FloorAnalysis floor( compilation, pTest );
        floor.render( constructPath( inputFile, "__floor.html" ) );
        
        Blueprint::Visibility visibility( floor );
        visibility.render( constructPath( inputFile, "__vis.html" ) );
        
    }
    catch( std::exception& ex )
    {
        std::cout << "Error with test file: " << inputFile.string() << " : " << ex.what() << std::endl;
    }
}

TEST( CGAL, CompilerTestFiles )
{
    //{
    //    char c;
    //    std::cin >> c;
    //}
    
    std::vector< boost::filesystem::path > testFiles;
    for( boost::filesystem::directory_iterator iter( testFilesFolderPath / "testfiles" );
        iter != boost::filesystem::directory_iterator(); ++iter )
    {
        const boost::filesystem::path& filePath = *iter;
        if( !boost::filesystem::is_directory( filePath ) )
        {
            if( boost::filesystem::extension( *iter ) == ".blu" )
            {
                if( !filePath.stem().empty() )
                {
                    testFiles.push_back( *iter );
                }
                else
                {
                    //make this recursive...
                }
            }
        }
    }
    
    for( const boost::filesystem::path& testFile : testFiles )
    {
        loadTest( testFile );
    }
}