
#ifndef NESTED_EDIT_17_DEC_2020
#define NESTED_EDIT_17_DEC_2020

#include "blueprint/editBase.h"

namespace Blueprint
{

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class EditNested : public EditBase
{
public:
    typedef boost::shared_ptr< EditNested > Ptr;
    typedef std::map< Site::Ptr, EditNested::Ptr > Map;
    typedef std::map< Feature::Ptr, IGlyph::PtrSet > FeatureGlyphMap;
    typedef std::map< const GlyphSpec*, IGlyph::Ptr > GlyphMap;

    EditNested( EditMain& editMain, IEditContext& parentContext, Site::Ptr pSpace, GlyphFactory& glyphFactory );
    virtual void interaction_update();

    virtual IEditContext* getParent() { return &m_parent; }
    virtual const Origin* getOrigin() const { return m_pSite.get(); }

    void matchFeatures();
    bool owns( IGlyph* pGlyph ) const;
    bool owns( const GlyphSpec* pGlyphSpec ) const;
    IGlyph::Ptr getMainGlyph() const { return m_pMainGlyph; }
    virtual void cmd_delete( const std::set< IGlyph* >& selection );
    
    virtual GlyphSpecProducer* fromGlyph( IGlyph* pGlyph ) const;
    
protected:
    IEditContext& m_parent;
    IGlyph::Ptr m_pMainGlyph;
    FeatureGlyphMap m_features;
    GlyphMap m_glyphs;
    GlyphMap m_markup;
};

}

#endif NESTED_EDIT_17_DEC_2020/