#ifndef PROPERTY_18_09_2013
#define PROPERTY_18_09_2013

#include "glyphSpec.h"
#include "node.h"

#include "ed/ed.hpp"

#include "common/stl.hpp"
#include "common/compose.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/chrono.hpp>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>

namespace Blueprint
{

class Factory;
class Property;


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
class Property : public Node, public boost::enable_shared_from_this< Property >
{
public:
    typedef boost::shared_ptr< Property > Ptr;
    typedef boost::shared_ptr< const Property > PtrCst;
    typedef std::map< std::string, Ptr > PtrMap;
    
    static const std::string& TypeName();
    Property( Node::Ptr pParent, const std::string& strName );
    Property( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName );
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    virtual void init();
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const;
    void setStatement( const std::string& strStatement );
    const std::string& getValue() const { return m_strValue; }

private:
    std::string m_strValue;
};
/*
template< class T >
class PropertyT : public Node, public boost::enable_shared_from_this< PropertyT< T > >
{
public:
    typedef boost::shared_ptr< PropertyT > Ptr;
    typedef boost::shared_ptr< const PropertyT > PtrCst;
    typedef std::map< std::string, Ptr > PtrMap;
    
    PropertyT( Node::Ptr pParent, const std::string& strName )
    :   Node( pParent, strName )
    {
    }
    PropertyT( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName )
    :   Node( pOriginal, pParent, strName ),
        m_value( pOriginal->m_value )
    {
    }
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    virtual void init()
    {
        Node::init();
    }
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const
    {
        return Node::copy< PropertyT >( shared_from_this(), pParent, strName );
    }
    virtual void load( Factory& factory, const Ed::Node& node )
    {
        if( boost::optional< const Ed::Expression& > expression = 
            Ed::getFirstArgument< Ed::Expression >( node.statement ) )
        {
            std::istringstream is( expression.get() );
            is >> m_value;
        }
        Node::load( shared_from_this(), factory, node );
    }
    virtual void save( Ed::Node& node ) const
    {
        Node::save( node );
        node.statement.declarator.typeDeclaration =
            Ed::TypeDeclaration( std::string( "PropertyT" ) );
        node.statement.shortHand = Ed::ShortHand( getStatement() );
    }
    virtual std::string getStatement() const
    {
        std::istringstream os;
        os << m_value;
        return os.str();
    }

    const T& getValue() const 
    { 
        return m_value; 
    }
private:
    T m_value;
};*/

class RefPtr
{
    struct ReferenceResolver : public boost::static_visitor< Node::Ptr >
    {
        Node::Ptr m_ptr;
        ReferenceResolver( Node::Ptr pThis )
            :   m_ptr( pThis )
        {
        }
        
        Node::Ptr operator()( const Ed::Identifier& str ) const
        {
            return m_ptr->get< Node >( str );
        }
        Node::Ptr operator()( const Ed::RefActionType& type ) const
        {
            Node::Ptr pResult;
            switch( type )
            {
            case Ed::eRefUp:
                pResult = m_ptr->getParent();
                break;
            default:
                ASSERT( false );
                break;
            }
            return pResult;
        }
        Node::Ptr operator()( const Ed::Ref& ) const
        {
            THROW_RTE( "Invalid reference branch used in blueprint reference" );
            return Node::Ptr();
        }
    };
public:
    RefPtr( Node::Ptr pThis, const Ed::Reference& reference )
        :   m_pThis( pThis ),
            m_reference( reference )
    {

    }

    template< class T >
    boost::shared_ptr< T > get() const
    {
        Node::Ptr pIter = m_pThis.lock();
        for( Ed::Reference::const_iterator 
            i = m_reference.begin(),
            iEnd = m_reference.end(); i!=iEnd && pIter; ++i )
        {
            pIter = boost::apply_visitor( ReferenceResolver( pIter ), *i );
        }
        return boost::dynamic_pointer_cast< T >( pIter );
    }

private:
    Node::PtrWeak m_pThis;
    Ed::Reference m_reference;
};

static bool calculateReference( Node::Ptr pSource, Node::Ptr pTarget, Ed::Reference& result )
{
    bool bSuccess = false;

    //first calculate the paths to the root 
    typedef std::list< std::string > StringList;
    StringList sourcePath, targetPath;
    for( Node::Ptr pIter = pSource; pIter; pIter = pIter->getParent() )
        sourcePath.push_front( pIter->getName() );
    for( Node::Ptr pIter = pTarget; pIter; pIter = pIter->getParent() )
        targetPath.push_front( pIter->getName() );
    
    if( !sourcePath.empty() && !targetPath.empty() 
        && sourcePath.front() == targetPath.front() )
    {
        bSuccess = true;

        StringList::iterator 
            i1 = targetPath.begin(),
            i1End = targetPath.end(), 
            i2 = sourcePath.begin(),
            i2End = sourcePath.end();
        for( ; i1 != i1End && i2 != i2End && *i1 == *i2; ++i1, ++i2 )
        {}

        //generate the up part
        if( std::size_t uiUps = std::distance( i2, i2End ) )
        {
            result.resize( uiUps );
            std::fill_n( result.begin(), uiUps, Ed::eRefUp );
        }
        //generate the down part
        std::copy( i1, i1End, std::back_inserter( result ) );
    }

    return bSuccess;
}


class Reference : public Node, public boost::enable_shared_from_this< Reference >
{
public:
    typedef boost::shared_ptr< Reference > Ptr;
    typedef boost::shared_ptr< const Reference > PtrCst;
    typedef std::map< std::string, Ptr > PtrMap;
    
    static const std::string& TypeName();
    Reference( Node::Ptr pParent, const std::string& strName );
    Reference( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName );
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    virtual void init();
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const;

    const Ed::Reference& getValue() const;
    void setValue( const Ed::Reference& ref ) { m_reference = ref; }
private:
    Ed::Reference m_reference;
};

}

#endif //PROPERTY_18_09_2013