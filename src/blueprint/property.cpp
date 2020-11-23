#include "blueprint/property.h"
#include "blueprint/factory.h"

#include "ed/node.hpp"
#include "ed/nodeio.hpp"
#include "ed/serialise.hpp"

#include "common/assert_verify.hpp"

#include <boost/optional.hpp>

namespace Blueprint
{
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
const std::string& Property::TypeName()
{
    static const std::string strTypeName( "property" );
    return strTypeName;
}
Property::Property( Node::Ptr pParent, const std::string& strName )
    :   Node( pParent, strName )
{
}

Property::Property( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName )
    :   Node( pOriginal, pParent, strName ),
        m_strValue( pOriginal->m_strValue )
{

}

void Property::init()
{
    Node::init();
}

Node::Ptr Property::copy( Node::Ptr pParent, const std::string& strName ) const
{
    return Node::copy< Property >( shared_from_this(), pParent, strName );
}

void Property::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );
    
    if( boost::optional< const Ed::Shorthand& > shOpt = node.getShorty() )
    {
        Ed::IShorthandStream is( shOpt.get() );
        is >> m_strValue;
    }
}

void Property::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Node::save( node );
    
    if( !node.statement.shorthand ) node.statement.shorthand = Ed::Shorthand();
    Ed::OShorthandStream os( node.statement.shorthand.get() );
    os << m_strValue;
}

std::string Property::getStatement() const
{
    std::ostringstream os;
    {
        Ed::Shorthand sh;
        {
            Ed::OShorthandStream ossh( sh );
            ossh << m_strValue;
        }
        os << sh;
    }
    return os.str();
}

void Property::setStatement( const std::string& strStatement ) 
{ 
    if( m_strValue != strStatement )
    {
        m_strValue = strStatement; 
        setModified();
    }
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
const std::string& Reference::TypeName()
{
    static const std::string strTypeName( "reference" );
    return strTypeName;
}
Reference::Reference( Node::Ptr pParent, const std::string& strName )
:   Node( pParent, strName )
{
}
Reference::Reference( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName )
:   Node( pOriginal, pParent, strName ),
    m_reference( pOriginal->m_reference )
{
}
void Reference::init()
{
    Node::init();
}
Node::Ptr Reference::copy( Node::Ptr pParent, const std::string& strName ) const
{
    return Node::copy< Reference >( shared_from_this(), pParent, strName );
}
void Reference::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );

    if( boost::optional< const Ed::Shorthand& > shOpt = node.getShorty() )
    {
        Ed::IShorthandStream is( shOpt.get() );
        Ed::Ref ref;
        is >> ref;
        m_reference = ref.front();
        m_reference.erase( m_reference.begin() );
    }
}
void Reference::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );
    
    Node::save( node );

    if( !node.statement.shorthand ) node.statement.shorthand = Ed::Shorthand();
    Ed::OShorthandStream os( node.statement.shorthand.get() );
    Ed::Ref ref;
    ref.push_back( m_reference );
    ref.back().insert( ref.back().begin(), Ed::eRefUp );
    os << ref;
}
std::string Reference::getStatement() const
{
    std::ostringstream os;
    {
        Ed::Shorthand sh;
        {
            Ed::OShorthandStream ossh( sh );
            Ed::Ref ref;
            ref.push_back( m_reference );
            ossh << ref;
        }
        os << sh;
    }
    return os.str();
}

const Ed::Reference& Reference::getValue() const 
{ 
    return m_reference; 
}

}