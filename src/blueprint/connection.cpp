

#include "blueprint/connection.h"

#include "common/assert_verify.hpp"

namespace Blueprint
{

const std::string& Connection::TypeName()
{
    static const std::string strTypeName( "connect" );
    return strTypeName;
}

Connection::Connection( Site::Ptr pParent, const std::string& strName )
    :   Site( pParent, strName )
{
    
}

Connection::Connection( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName )
{
    
}

Node::Ptr Connection::copy( Node::Ptr pParent, const std::string& strName ) const
{
    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
    VERIFY_RTE( pSiteParent || !pParent );
    return Node::copy< Connection >( 
        boost::dynamic_pointer_cast< const Connection >( shared_from_this() ), pSiteParent, strName );
}

void Connection::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );
    Site::save( node );
}
    
std::string Connection::getStatement() const
{
    return Site::getStatement();
}

void Connection::init()
{
    Site::init();
}

void Connection::init( float x, float y )
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