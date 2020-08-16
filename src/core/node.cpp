#include "blueprint/node.h"
#include "blueprint/factory.h"

#include "common/assert_verify.hpp"

#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>

namespace Blueprint
{
    
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
Node::Node( Node::Ptr pParent, const std::string& strName )
    :   m_pParent( pParent ),
        m_strName( boost::to_lower_copy( strName ) ),
        m_iIndex( 0 )
{
}

Node::Node( Node::PtrCst pOriginal, Node::Ptr pNewParent, const std::string& strName )
    :   m_pParent( pNewParent ),
        m_strName( boost::to_lower_copy( strName ) ),
        m_iIndex( pOriginal->m_iIndex ),
        m_passThroughMetaData( pOriginal->m_passThroughMetaData )
{
}

Node::~Node()
{
}

std::string Node::generateNewNodeName( const std::string& strPrefix ) const
{
    std::string strNewKey;
    size_t iIndex = m_childrenOrdered.size();
    const std::string strPrefixLowered = boost::to_lower_copy( strPrefix );
    do
    {
        std::ostringstream os;
        os << strPrefixLowered << '_' << std::setfill( '0' ) << std::setw( 4 ) << iIndex++;
        strNewKey = os.str();
    }while( get< Node >( strNewKey ) );
    return strNewKey;
}

void Node::setModified()
{
    m_lastModifiedTick.update();
}
void Node::init()
{
    for_each( boost::bind( &Node::init, _1 ) );
    setModified();
}

void Node::load( Node::Ptr pThis, Factory& factory, const Ed::Node& node )
{
    VERIFY_RTE_MSG( node.statement.declarator.identifier, "Node with no identifier" );
    //m_strName = node.statement.declarator.identifier.get();

    for( Ed::Node::Vector::const_iterator 
        i = node.children.begin(), iEnd = node.children.end(); i!=iEnd; ++i )
    {
        VERIFY_RTE_MSG( i->statement.declarator.identifier, "Node with no identifier" );
        if( Node::Ptr pNewNode = factory.load( pThis, *i ) )
        {
            pNewNode->m_iIndex = m_childrenOrdered.size();
            m_childrenOrdered.push_back( pNewNode );
            m_children.insert( std::make_pair( i->statement.declarator.identifier.get(), pNewNode ) );
        }
        else
        {
            m_passThroughMetaData.children.push_back( *i ); //deep copy
        }
    }
    setModified();
}

void Node::save( Ed::Node& node ) const
{
    node.statement.declarator.identifier = getName();
    for( PtrVector::const_iterator i = m_childrenOrdered.begin(), 
        iEnd = m_childrenOrdered.end(); i!=iEnd; ++i )
    {
        Ed::Node n;
        n.statement.declarator.identifier = (*i)->getName();
        (*i)->save( n );
        node.children.push_back( n );
    }
    
    //save meta data
    for( Ed::Node::Vector::const_iterator 
        i = m_passThroughMetaData.children.begin(), 
        iEnd = m_passThroughMetaData.children.end(); i!=iEnd; ++i )
    {
        node.children.push_back( *i );
    }
}

bool Node::add( Node::Ptr pNewNode )
{
    bool bInserted = false;
    PtrMap::const_iterator iFind = m_children.find( pNewNode->getName() );
    if( iFind == m_children.end() )
    {
        pNewNode->m_iIndex = m_childrenOrdered.size();
        m_childrenOrdered.push_back( pNewNode );
        m_children.insert( std::make_pair( pNewNode->getName(), pNewNode ) );
        setModified();
        bInserted = true;
    }
    return bInserted;
}
void Node::remove( Node::Ptr pNode )
{
    PtrVector::iterator iFind = 
        std::find( m_childrenOrdered.begin(), m_childrenOrdered.end(), pNode );
    PtrMap::const_iterator iFindKey = m_children.find( pNode->getName() );
    VERIFY_RTE( iFind != m_childrenOrdered.end() && iFindKey != m_children.end() );
    m_children.erase( iFindKey );
    //erase and update the later indices
    for( PtrVector::iterator i = m_childrenOrdered.erase( iFind ),
        iBegin = m_childrenOrdered.begin(),
        iEnd = m_childrenOrdered.end(); i!=iEnd; ++i )
        (*i)->m_iIndex = (i-iBegin);
    setModified();
}
/*
void Node::removeOptional( Ptr pNode )
{
    PtrVector::iterator iFind = 
        std::find( m_childrenOrdered.begin(), m_childrenOrdered.end(), pNode );
    PtrMap::const_iterator iFindKey = m_children.find( pNode->getName() );
    VERIFY_RTE( (iFind != m_childrenOrdered.end() && iFindKey != m_children.end()) || 
        !(iFind != m_childrenOrdered.end() && iFindKey != m_children.end()));
    if( iFind != m_childrenOrdered.end() && iFindKey != m_children.end() )
        remove( pNode );
}*/

boost::optional< std::string > Node::getPropertyString( const std::string& strKey ) const
{
    boost::optional< std::string > result;
    PtrMap::const_iterator iFind = m_children.find( strKey );
    if( iFind != m_children.end() )
    {
        if( Property::Ptr pProperty = boost::dynamic_pointer_cast< Property >( iFind->second ) )
            result = pProperty->getValue();
    }
    return result;
}

}