#ifndef NODE_21_09_2013
#define NODE_21_09_2013

#include "glyphSpec.h"

#include "ed/node.hpp"

#include "common/stl.hpp"
#include "common/compose.hpp"
#include "common/assert_verify.hpp"
#include "common/tick.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/chrono.hpp>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <iomanip>

namespace Blueprint
{
class Factory;
class Property;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
class Node
{
    Node( const Node& cpy );
    Node& operator=( const Node& cpy );
public:
    typedef boost::shared_ptr< Node > Ptr;
    typedef boost::shared_ptr< const Node > PtrCst;
    typedef boost::weak_ptr< Node > PtrWeak;
    typedef boost::weak_ptr< const Node > PtrCstWeak;
    typedef std::map< std::string, Ptr > PtrMap;
    typedef std::set< Ptr > PtrSet;
    typedef std::set< PtrCst > PtrCstSet;
    typedef std::list< Ptr > PtrList;
    typedef std::vector< Ptr > PtrVector;
    typedef std::vector< PtrCst > PtrCstVector;
    
    template< class T >
    struct ConvertPtrType
    {
        boost::shared_ptr< T > operator()( Node::Ptr p ) const
        {
            return boost::dynamic_pointer_cast< T >( p );
        }
    };

    Node( Node::Ptr pParent, const std::string& strName );
    Node( Node::PtrCst pOriginal, Node::Ptr pNewParent, const std::string& strName );
    virtual ~Node();
    virtual Node::PtrCst getPtr() const=0;
    virtual Node::Ptr getPtr()=0;

protected:
    void load( Ptr pThis, Factory& factory, const Ed::Node& node );
    
    template< class T, class TParentPtrType >
    inline boost::shared_ptr< T > copy( boost::shared_ptr< const T > pThis, TParentPtrType pNewParent, const std::string& strName ) const
    {
        boost::shared_ptr< T > pCopy( new T( pThis, pNewParent, strName ) );
        for( PtrVector::const_iterator 
            i = m_childrenOrdered.begin(),
            iEnd = m_childrenOrdered.end(); i!=iEnd; ++i )
        {
            VERIFY_RTE( pCopy->add( (*i)->copy( pCopy, (*i)->getName() ) ) );
        }
        
        //copy meta data
        pCopy->m_passThroughMetaData = m_passThroughMetaData;
        
        pCopy->init();
        return pCopy;
    }
public:
    const std::string& getName()                const { return m_strName; }
    Ptr getParent()                             const { return m_pParent.lock(); }
    const PtrVector& getChildren()              const { return m_childrenOrdered; }
    std::size_t size()                          const { return m_childrenOrdered.size(); }
    const Timing::UpdateTick& getLastModifiedTick()     const { return m_lastModifiedTick; }
    std::size_t getIndex()                      const { return m_iIndex; }
    virtual std::string getStatement()          const = 0;

    void setModified();
    virtual void init();
    virtual Ptr copy( Node::Ptr pParent, const std::string& strName ) const=0;
    virtual void load( Factory& factory, const Ed::Node& node ) = 0;
    virtual void save( Ed::Node& node ) const;
    virtual bool add( Ptr pNewNode );
    virtual void remove( Ptr pNode );
    //void removeOptional( Ptr pNode );

    std::string generateNewNodeName( const std::string& strPrefix ) const;
    
    template< class T >
    boost::optional< T > getProperty( const std::string& strKey ) const
    {
        boost::optional< T > result;
        typename PtrMap::const_iterator iFind = m_children.find( strKey );
        if( iFind != m_children.end() )
        {
            if( typename Property::Ptr pProperty = boost::dynamic_pointer_cast< Property >( iFind->second ) )
            {
                std::istringstream is( pProperty->getValue() );
                T value = T();
                is >> value;
                result = value;
            }
        }
        return result;
    }
    boost::optional< std::string > getPropertyString( const std::string& strKey ) const;
    
    template< class T >
    boost::shared_ptr< T > get( const std::string& strKey ) const
    {
        boost::shared_ptr< T > pResultPtr;
        typename PtrMap::const_iterator iFind = m_children.find( strKey );
        if( iFind != m_children.end() )
            pResultPtr = boost::dynamic_pointer_cast< T >( iFind->second );
        return pResultPtr;
    }

    template< class TPredicate >
    inline void for_each( TPredicate& predicate ) const
    {
        generics::for_each_second( m_children, predicate );
    }
    
    template< class TFunctor, class TPredicateCutOff >
    struct DepthFirstRecursion
    {
        TFunctor& m_functor;
        TPredicateCutOff& m_cutoffPredicate;
        DepthFirstRecursion( TFunctor& functor, TPredicateCutOff& cutoffPredicate ) 
            : m_functor( functor ), m_cutoffPredicate( cutoffPredicate ) {}
        inline void operator()( Node::Ptr p ) const
        {
            m_functor( p );
            if( !m_cutoffPredicate( p ) )
                p->for_each( DepthFirstRecursion< TFunctor, TPredicateCutOff >( m_functor, m_cutoffPredicate ) );
        }
    };

    template< class TFunctor, class TPredicateCutOff >
    void for_each_recursive( TFunctor& functor, TPredicateCutOff& cutoffPredicate = generics::_not( generics::all() ) ) const
    {
        generics::for_each_second( m_children, DepthFirstRecursion< TFunctor, TPredicateCutOff >( functor, cutoffPredicate ) );
    }

    const Ed::Node& getMetaData() const { return m_passThroughMetaData; }
protected:
    PtrWeak m_pParent;

private:
    const std::string m_strName;
    PtrVector m_childrenOrdered;
    PtrMap m_children;
    std::size_t m_iIndex;
    Timing::UpdateTick m_lastModifiedTick;
    Ed::Node m_passThroughMetaData;
};

}

#endif //NODE_21_09_2013