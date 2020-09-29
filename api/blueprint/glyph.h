#ifndef GLYPH_16_09_2013
#define GLYPH_16_09_2013

#include "buffer.h"
#include "node.h"
#include "glyphSpec.h"

//#include "common/stl.h"

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>

#include <vector>
#include <set>

namespace Blueprint
{

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class IGlyph : public boost::enable_shared_from_this< IGlyph >
{
public:
    typedef boost::shared_ptr< IGlyph > Ptr;
    typedef std::vector< Ptr > PtrVector;
    typedef std::set< Ptr > PtrSet;
    IGlyph( const GlyphSpec* pGlyphSpec, Ptr pParent )
        :   m_pGlyphSpec( pGlyphSpec ),
            m_pParent( pParent )
    {
    }
    virtual ~IGlyph(){}
    virtual void update() = 0;
    const GlyphSpec* getGlyphSpec() const { return m_pGlyphSpec; }
    Ptr getParent() const { return m_pParent; }
protected:
    const GlyphSpec* m_pGlyphSpec;
    Ptr m_pParent;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class GlyphControlPoint : public IGlyph
{
public:
    GlyphControlPoint( ControlPoint* pControlPoint, IGlyph::Ptr pParent )
        :   IGlyph( pControlPoint, pParent )
    {}
    const ControlPoint* getControlPoint() const { return dynamic_cast< const ControlPoint* >( m_pGlyphSpec ); }
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class GlyphImage : public IGlyph
{
    friend class SpaceGlyphs;
    friend class Interaction;
public:
    GlyphImage( ImageSpec* pImageSpec, IGlyph::Ptr pParent )
        :   IGlyph( pImageSpec, pParent )
    {}
    
    const ImageSpec* getImageSpec() const { return dynamic_cast< const ImageSpec* >( m_pGlyphSpec ); }
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class GlyphPath : public IGlyph
{
public:
    GlyphPath( MarkupPath* pMarkupPath, IGlyph::Ptr pParent )
        :   IGlyph( pMarkupPath, pParent )
    {}
    const MarkupPath* getMarkupPath() const { return dynamic_cast< const MarkupPath* >( m_pGlyphSpec ); }
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class GlyphText : public IGlyph
{
public:
    GlyphText( MarkupText* pMarkupText, IGlyph::Ptr pParent )
        :   IGlyph( pMarkupText, pParent )
    {}
    const MarkupText* getMarkupText() const { return dynamic_cast< const MarkupText* >( m_pGlyphSpec ); }
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class GlyphFactory
{
public:
    virtual IGlyph::Ptr createControlPoint( ControlPoint* pControlPoint, IGlyph::Ptr pParent ) = 0;
    virtual IGlyph::Ptr createImage( ImageSpec* pImageSpec, IGlyph::Ptr pParent ) = 0;
    virtual IGlyph::Ptr createMarkupPath( MarkupPath* pMarkupPath, IGlyph::Ptr pParent ) = 0;
    virtual IGlyph::Ptr createMarkupText( MarkupText* pMarkupText, IGlyph::Ptr pParent ) = 0;
};

}

#endif //GLYPH_16_09_2013