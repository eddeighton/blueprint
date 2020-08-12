

//#include "common/edsUnitTestWrapper.h"

#include "ed/file.hpp"

#include "blueprint/factory.h"
#include "blueprint/toolbox.h"
#include "blueprint/site.h"

#include "core/serialisation.h"

#include "common/file.hpp"

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>

#include <fstream>
#include <string>
#include <sstream>


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

}

TEST( BlueprintTests, Check2 )
{
    const std::string strFile1 = "../testdata/test1.blu";
    const std::string strFile2 = "../testdata/test1.blu";

    Blueprint::Factory factory;
    Blueprint::Site::Ptr pTest = factory.load( strFile1 );
    ASSERT_TRUE( pTest );

    pTest = boost::dynamic_pointer_cast< Blueprint::Site >( 
        pTest->copy( Blueprint::Site::Ptr(), pTest->Node::getName() ) );

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

}

TEST( Serialisation, Points )
{
    using namespace Ed;
    wykobi::point2d< float > p = wykobi::make_point< float >( 123.567f, 321.765f );
    std::ostringstream os;
    os << p;

    std::istringstream is( os.str() );

    wykobi::point2d< float > result;
    is >> result;
    ASSERT_FLOAT_EQ( p.x, result.x );
    ASSERT_FLOAT_EQ( p.y, result.y );
}

TEST( Serialisation, Polygons )
{
    using namespace Ed;
    wykobi::polygon< float, 2 > p = wykobi::make_polygon< float >( 
        wykobi::make_circle< float >( 0.0f,0.0f,123.456f)  );
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
        ASSERT_FLOAT_EQ( i->x, j->x );
        ASSERT_FLOAT_EQ( i->y, j->y );
    }
}

TEST( Toolbox, Check1 )
{
    Blueprint::Toolbox::Ptr pToolbox( new Blueprint::Toolbox( "C:/WORKSPACE/Blueprint/data" ) );
    ASSERT_TRUE( pToolbox->getPalette( "data" ) );
}