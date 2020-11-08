#include "blueprint/factory.h"
#include "blueprint/property.h"

#include "blueprint/basicFeature.h"
#include "blueprint/blueprint.h"
#include "blueprint/basicarea.h"

#include "ed/node.hpp"

#include "common/assert_verify.hpp"
#include "common/variant_utils.hpp"

#include <boost/variant.hpp>
#include <boost/filesystem/path.hpp>

#include <fstream>

namespace Blueprint
{

Site::Ptr Factory::create( const std::string& strName )
{
    Site::Ptr pNewBlueprint( new Blueprint( strName ) );
    return pNewBlueprint;
}

Node::Ptr Factory::load( Node::Ptr pParent, const Ed::Node& node )
{
    Node::Ptr pResult;

    if( node.statement.tagList && node.statement.declarator.identifier )
    {
        const Ed::TagList& tags = node.statement.tagList.get();
        const Ed::Identifier identity = node.statement.declarator.identifier.get();

        //match based on the first match
        for( Ed::TagList::const_iterator i = tags.begin(),
            iEnd = tags.end(); i!=iEnd && !pResult; ++i )
        {
            if( boost::optional< const Ed::Identifier& > idOpt = 
                boost::apply_visitor( boost::TypeAccessor< const Ed::Identifier >(), *i ) )
            {
                const Ed::Identifier& id = idOpt.get();
                
                if( id == Area::TypeName() )
                {
                    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
                    ASSERT( !pParent || pSiteParent );
                    pResult = Area::Ptr( new Area( pSiteParent, identity ) );
                }
                else if( id == Clip::TypeName() )
                {
                    pResult = Clip::Ptr( new Clip( pParent, identity ) );
                }
                else if( id == Connection::TypeName() )
                {
                    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
                    ASSERT( !pParent || pSiteParent );
                    pResult = Connection::Ptr( new Connection( pSiteParent, identity ) );
                }
                else if( id == Blueprint::TypeName() )
                {
                    ASSERT( !pParent );
                    pResult = Blueprint::Ptr( new Blueprint( identity ) );
                }
                else if( id == Feature::TypeName() )
                {
                    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
                    Feature::Ptr pFeatureParent = boost::dynamic_pointer_cast< Feature >( pParent );
                    ASSERT( !pParent || ( pSiteParent || pFeatureParent ) );
                    pResult = Feature::Ptr( new Feature( pParent, identity ) );
                }
                else if( id == Reference::TypeName() )
                {
                    ASSERT( pParent );
                    pResult = Reference::Ptr( new Reference( pParent, identity ) );
                }
                else if( id == Feature_Point::TypeName() )
                {
                    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
                    Feature::Ptr pFeatureParent = boost::dynamic_pointer_cast< Feature >( pParent );
                    ASSERT( !pParent || ( pSiteParent || pFeatureParent ) );
                    pResult = Feature_Point::Ptr( new Feature_Point( pParent, identity ) );
                }
                else if( id == Feature_ContourPoint::TypeName() )
                {
                    Feature_Contour::Ptr pFeatureParent = boost::dynamic_pointer_cast< Feature_Contour >( pParent );
                    ASSERT( pFeatureParent );
                    pResult = Feature_ContourPoint::Ptr( new Feature_ContourPoint( pFeatureParent, identity ) );
                }
                else if( id == Feature_ContourSegment::TypeName() )
                {
                    Feature_Contour::Ptr pFeatureParent = boost::dynamic_pointer_cast< Feature_Contour >( pParent );
                    ASSERT( pFeatureParent );
                    pResult = Feature_ContourSegment::Ptr( new Feature_ContourSegment( pFeatureParent, identity ) );
                }
                else if( id == Feature_Contour::TypeName() )
                {
                    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
                    Feature::Ptr pFeatureParent = boost::dynamic_pointer_cast< Feature >( pParent );
                    ASSERT( !pParent || ( pSiteParent || pFeatureParent ) );
                    pResult = Feature_Contour::Ptr( new Feature_Contour( pParent, identity ) );
                }
                else if( id == Property::TypeName() )
                {
                    pResult = Property::Ptr( new Property( pParent, identity ) );
                }
            }
        }
    }

    if( pResult )
    {
        pResult->load( *this, node );
        pResult->init();
    }

    return pResult;
}

Site::Ptr Factory::load( const std::string& strFilePath )
{
    Site::Ptr pNewBlueprint;
    
    Ed::Node node;
    {
        Ed::BasicFileSystem filesystem;
        Ed::File edFile( filesystem, strFilePath );

        edFile.expandShorthand();
        edFile.removeTypes();

        edFile.toNode( node );
    }
    if( !node.children.empty() )
    {
        pNewBlueprint = boost::dynamic_pointer_cast< Site >( load( Node::Ptr(), node.children.front() ) );
    }

    return pNewBlueprint;
}

void Factory::load( const std::string& strFilePath, Node::PtrVector& results )
{
    Ed::Node node;
    {
        Ed::BasicFileSystem filesystem;
        Ed::File edFile( filesystem, strFilePath );

        edFile.expandShorthand();
        edFile.removeTypes();

        edFile.toNode( node );
    }

    for( auto i = node.children.begin(),
        iEnd = node.children.end(); i!=iEnd; ++i )
    {
        if( Node::Ptr pNode = load( Node::Ptr(), *i ) )
        {
            results.push_back( pNode );
        }
    }
}

void Factory::save( Site::Ptr pBlueprint, const std::string& strFilePath )
{
    boost::filesystem::path filePath = strFilePath;
    const std::string strName = filePath.filename().replace_extension( "" ).string();
    
    VERIFY_RTE_MSG( !strName.empty(), "Invalid file name specified: " << strFilePath );
    
    Ed::Node node( Ed::Statement( Ed::Declarator( ( Ed::Identifier( strName ) ) ) ) );
    
    pBlueprint->save( node );
    
    node.statement.declarator.identifier = strName;
    
    std::ofstream of( strFilePath );
    of << node;
}


}
