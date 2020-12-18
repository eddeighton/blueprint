#ifndef BLUEPRINT_25_09_2013
#define BLUEPRINT_25_09_2013

#include "basicFeature.h"

#include "blueprint/buffer.h"
#include "blueprint/property.h"
#include "blueprint/site.h"

#include "ed/node.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <list>
#include <map>
#include <string>
#include <vector>

namespace Blueprint
{

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Blueprint : public Site, public boost::enable_shared_from_this< Blueprint >
{
    Blueprint& operator=( const Blueprint& );
    
public:
    typedef boost::shared_ptr< Blueprint > Ptr;
    typedef boost::shared_ptr< const Blueprint > PtrCst;
    typedef boost::weak_ptr< Blueprint > WeakPtr;
    typedef std::list< Ptr > PtrList;
    typedef std::vector< Ptr > PtrVector;
    typedef std::list< WeakPtr > WeakPtrList;
    typedef std::map< Ptr, Ptr > PtrMap;
    
    static const std::string& TypeName();
    Blueprint( const std::string& strName );
    Blueprint( PtrCst pOriginal, Node::Ptr pNotUsed, const std::string& strName );
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void init();
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const { return ""; }
    
    //GlyphSpec
    virtual const GlyphSpec* getParent() const { return 0u; }
    virtual const std::string& getName() const { return Node::getName(); }

    //Origin
    virtual const Transform& getTransform() const 
    { 
        static const Transform m_transform;
        return m_transform; 
    }
    virtual void setTransform( const Transform& transform ) 
    { 
        ASSERT( false );
    }
    
    virtual bool canEdit()                  const { return false; }
    virtual void set( Float fX, Float fY ) {}

    //Site
    virtual void evaluate( const EvaluationMode& mode, EvaluationResults& results );
    virtual Feature_Contour::Ptr getContour() const { return ( Feature_Contour::Ptr() ); }
    
    virtual bool add( Node::Ptr pNewNode );
    virtual void remove( Node::Ptr pNode );

};



}

#endif //BLUEPRINT_25_09_2013