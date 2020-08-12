
#include <fstream>
#include <string>
#include <sstream>

#include <boost/filesystem.hpp>

#include "common/edsUnitTestWrapper.h"
#include "common/filepath.h"

#include "parser/edFile.h"

#include "blueprint/factory.h"
#include "blueprint/toolbox.h"
#include "blueprint/site.h"

#include "core/serialisation.h"


TEST( BlueprintTests, Check )
{
    const std::string strFile1 = "C:/WORKSPACE/Blueprint/data/test1.blu";
    const std::string strFile2 = "C:/WORKSPACE/Blueprint/data/copies/test1.blu";

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
    
    Parser::EdFile e1( path1.string() ), e2( path2.string() );
    
    ASSERT_EQ( e1.getContents(), e2.getContents() );

}

TEST( BlueprintTests, Check2 )
{
    const std::string strFile1 = "C:/WORKSPACE/Blueprint/data/test1.blu";
    const std::string strFile2 = "C:/WORKSPACE/Blueprint/data/copies/test1.blu";

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
    
    Parser::EdFile e1( path1.string() ), e2( path2.string() );
    
    ASSERT_EQ( e1.getContents(), e2.getContents() );

}

TEST( Serialisation, Points )
{
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