
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
    Clip( Site::Ptr pParent, const std::string& strName );
    Clip( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName );
    ~Clip(){};
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    
    virtual void init() { Site::init(); }
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void load( Factory& factory, const Ed::Node& node ) { return Node::load( shared_from_this(), factory, node ); }
    virtual void save( Ed::Node& node ) const { return Node::save( node ); }
    virtual std::string getStatement() const { return getName(); }
    
    //GlyphSpec
    virtual const std::string& getName() const { return Node::getName(); }
    virtual const GlyphSpec* getParent() const { return 0u; }

    //Origin
    virtual void set( float fX, float fY ){}
    virtual bool canEdit() const { return false; }
    
    //Site
    virtual Feature_Contour::Ptr getContour() const { return ( Feature_Contour::Ptr() ); }
    virtual boost::optional< Polygon2D > getContourPolygon() { return ( Polygon2D() ); }
};
}

#endif //CLIP_23_NOV_2020