#ifndef GLYPHSPEC_18_09_2013
#define GLYPHSPEC_18_09_2013

#include "transform.h"
#include "buffer.h"

#include <boost/shared_ptr.hpp>

#include <list>
#include <vector>
#include <map>
#include <set>
#include <string>

namespace Parser
{
    struct EdNode;
}

namespace Blueprint
{
    
class Factory;
class Blueprint;

class GlyphSpec
{
public:
    virtual ~GlyphSpec(){}
    virtual const GlyphSpec* getParent() const = 0;
    //virtual const std::string& getName() const = 0;
    virtual bool canEdit() const = 0;
};

using MarkupPoint = std::pair< Float, Float >;
using MarkupPolygon = std::vector< MarkupPoint >;

class MarkupPolygonGroup : public GlyphSpec
{
public:
    typedef std::list< MarkupPolygonGroup* > List;
    
    virtual bool isPolygonsFilled() const = 0;
    virtual std::size_t getTotalPolygons() const = 0;
    virtual void getPolygon( std::size_t szIndex, MarkupPolygon& polygon ) const = 0;
    
    virtual bool canEdit() const { return false; }
};

class MarkupText : public GlyphSpec
{
public:
    typedef std::list< MarkupText* > List;
    enum TextType
    {
        eImportant,
        eUnimportant
    };
    virtual const std::string& getText() const = 0;
    virtual TextType getType() const = 0;
    virtual Float getX() const = 0;
    virtual Float getY() const = 0;
    virtual bool canEdit() const { return false; }
};

class GlyphSpecInteractive : public GlyphSpec
{
public:
    virtual void set( Float fX, Float fY ) = 0;
};

class ControlPoint : public GlyphSpecInteractive
{
public:
    typedef std::list< ControlPoint* > List;

    virtual Float getX() const = 0;
    virtual Float getY() const = 0;
};

class Origin : public GlyphSpecInteractive
{
public:
    virtual const Transform& getTransform() const { return m_transform; }
    
    virtual const MarkupPolygonGroup* getMarkupContour() const = 0;
    
protected:
    Transform m_transform;
};

/*
class ImageSpec : public GlyphSpecInteractive
{
public:
    //virtual Float getX() const = 0;
    //virtual Float getY() const = 0;
    virtual Float getOffsetX() const = 0;
    virtual Float getOffsetY() const = 0;
    virtual NavBitmap::Ptr getBuffer() const = 0;
};
*/
}

#endif //GLYPHSPEC_18_09_2013