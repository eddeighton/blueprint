
#ifndef CLIP_23_NOV_2020
#define CLIP_23_NOV_2020

#include "spaceUtils.h"
#include "basicFeature.h"

#include "blueprint/buffer.h"
#include "blueprint/site.h"

#include "wykobi.hpp"

#include <boost/optional.hpp>

#include <string>
#include <cmath>
#include <memory>

namespace Blueprint
{
    
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Clip : public Site, public boost::enable_shared_from_this< Clip >
{
public:
    typedef boost::shared_ptr< Clip > Ptr;
    typedef boost::shared_ptr< const Clip > PtrCst;

    static const std::string& TypeName();
    Clip( Node::Ptr pParent, const std::string& strName );
    Clip( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName );
    ~Clip(){};
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    
    virtual void init() { Site::init(); }
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const { return Node::copy( shared_from_this(), pParent, strName ); }
    virtual void load( Factory& factory, const Ed::Node& node ) { return Node::load( shared_from_this(), factory, node ); }
    virtual void save( Ed::Node& node ) const { return Node::save( node ); }
    virtual std::string getStatement() const { return getName(); }
    
    virtual bool canEvaluate( const Site::PtrVector& evaluated ) const { ASSERT( false ); return true; }
    virtual EvaluationResult evaluate( const EvaluationMode& mode, DataBitmap& data ) 
    { 
        EvaluationResult result; 
        return result; 
    }
    
    //GlyphSpec
    virtual const std::string& getName() const { return Node::getName(); }
    virtual const GlyphSpec* getParent() const { return 0u; }

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
    virtual const MarkupPath* getPolygon()  const { return nullptr; }
    
    virtual void set( float fX, float fY ){}
    virtual bool canEdit() const { return false; }
};
}

#endif //CLIP_23_NOV_2020