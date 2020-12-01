
#include "blueprint/space.h"

namespace Blueprint
{

const std::string& Space::TypeName()
{
    static const std::string strTypeName( "space" );
    return strTypeName;
}

Space::Space( Site::Ptr pParent, const std::string& strName )
    :   Site( pParent, strName )
{
    
}

Space::Space( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName )
{
    
}

Node::Ptr Space::copy( Node::Ptr pParent, const std::string& strName ) const
{
    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
    VERIFY_RTE( pSiteParent || !pParent );
    return Node::copy< Space >( 
        boost::dynamic_pointer_cast< const Space >( shared_from_this() ), pSiteParent, strName );
}

void Space::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );
    Site::save( node );
}
    
std::string Space::getStatement() const
{
    return Site::getStatement();
}

void Space::init()
{
    Site::init();
}

void Space::init( float x, float y )
{
    //if( bEmptyContour )
    {
        m_pContour = Feature_Contour::Ptr( new Feature_Contour( shared_from_this(), "contour" ) );
        m_pContour->init();
        add( m_pContour );
    }
    
    Site::init();
    
    m_transform.setTranslation( Map_FloorAverage()( x ), Map_FloorAverage()( y ) );
}
    
}