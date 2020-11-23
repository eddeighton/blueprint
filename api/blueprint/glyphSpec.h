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
    virtual const std::string& getName() const = 0;
    virtual bool canEdit() const = 0;
};

class MarkupPath : public GlyphSpec
{
public:
    typedef std::list< MarkupPath* > List;
    struct Cmd
    {
        enum PathCmds : unsigned int //from agg directly - agg_basics.h
        {
            path_cmd_stop     = 0,        //----path_cmd_stop    
            path_cmd_move_to  = 1,        //----path_cmd_move_to 
            path_cmd_line_to  = 2,        //----path_cmd_line_to 
            path_cmd_curve3   = 3,        //----path_cmd_curve3  
            path_cmd_curve4   = 4,        //----path_cmd_curve4  
            path_cmd_curveN   = 5,        //----path_cmd_curveN
            path_cmd_catrom   = 6,        //----path_cmd_catrom
            path_cmd_ubspline = 7,        //----path_cmd_ubspline
            path_cmd_end_poly = 0x0F,     //----path_cmd_end_poly
            path_cmd_mask     = 0x0F      //----path_cmd_mask   
        };

        enum PathFlags
        {
            path_flags_none  = 0,         //----path_flags_none 
            path_flags_ccw   = 0x10,      //----path_flags_ccw  
            path_flags_cw    = 0x20,      //----path_flags_cw   
            path_flags_close = 0x40,      //----path_flags_close
            path_flags_mask  = 0xF0       //----path_flags_mask 
        };

        float x,y;
        unsigned int cmd;
        Cmd( float _x, float _y, unsigned int _cmd )
            : x( _x ), y( _y ), cmd( _cmd )
        {}
    };
    typedef std::vector< Cmd > PathCmdVector;
    virtual const PathCmdVector& getCmds() const = 0;
    virtual bool canEdit() const { return false; }
};

class MarkupPolygonGroup : public GlyphSpec
{
public:
    typedef std::list< MarkupPolygonGroup* > List;
    
    using Point = std::pair< float, float >;
    using Polygon = std::vector< Point >;
    
    virtual std::size_t getTotalPolygons() const = 0;
    virtual void getPolygon( std::size_t szIndex, Polygon& polygon ) const = 0;
    
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
    virtual float getX() const = 0;
    virtual float getY() const = 0;
    virtual bool canEdit() const { return false; }
};

class GlyphSpecInteractive : public GlyphSpec
{
public:
    virtual void set( float fX, float fY ) = 0;
};

class ControlPoint : public GlyphSpecInteractive
{
public:
    typedef std::list< ControlPoint* > List;

    virtual float getX() const = 0;
    virtual float getY() const = 0;
};

class Origin : public GlyphSpecInteractive
{
public:
    virtual const Transform& getTransform() const = 0;
    virtual void setTransform( const Transform& transform ) = 0;
    virtual const MarkupPath* getPolygon() const = 0;
};

/*
class ImageSpec : public GlyphSpecInteractive
{
public:
    //virtual float getX() const = 0;
    //virtual float getY() const = 0;
    virtual float getOffsetX() const = 0;
    virtual float getOffsetY() const = 0;
    virtual NavBitmap::Ptr getBuffer() const = 0;
};
*/
}

#endif //GLYPHSPEC_18_09_2013