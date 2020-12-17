
#ifndef BASE_EDIT_17_DEC_2020
#define BASE_EDIT_17_DEC_2020

#include "blueprint/buffer.h"
#include "blueprint/site.h"
#include "blueprint/glyph.h"
#include "blueprint/editInteractions.h"

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <vector>
#include <set>

namespace Blueprint
{


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class EditNested;
class EditMain;

class EditBase : public IEditContext
{
public:
    typedef boost::shared_ptr< EditBase > Ptr;

protected:
    EditBase( EditMain& editMain, GlyphFactory& glyphFactory, Site::Ptr pSite );
public:
    
    //IEditContext
    virtual void activated();
    virtual IInteraction::Ptr interaction_start( ToolMode toolMode, float x, float y, float qX, float qY, IGlyph* pHitGlyph, const std::set< IGlyph* >& selection );
    virtual IInteraction::Ptr interaction_draw( ToolMode toolMode, float x, float y, float qX, float qY, Site::Ptr pSite );

    virtual IEditContext* getNestedContext( const std::vector< IGlyph* >& candidates );
    virtual IEditContext* getParent() { return 0u; }
    virtual bool isSiteContext( Site::Ptr pSite ) const { return pSite == m_pSite; }
    virtual IEditContext* getSiteContext( Site::Ptr pSite );
    virtual bool canEdit( IGlyph* pGlyph, ToolType toolType, ToolMode toolMode ) const;
    virtual const Origin* getOrigin() const { return 0u; }
    
    void interaction_evaluate();
    virtual void interaction_update();
    virtual void interaction_end( IInteraction* pInteraction );

    virtual GlyphSpecProducer* fromGlyph( IGlyph* pGlyph ) const;

    //command handling
    virtual void cmd_delete( const std::set< IGlyph* >& selection );
    virtual Node::Ptr cmd_cut( const std::set< IGlyph* >& selection );
    virtual Node::Ptr cmd_copy( const std::set< IGlyph* >& selection );
    virtual IInteraction::Ptr cmd_paste( Node::Ptr pPaste, float x, float y, float qX, float qY );
    virtual IInteraction::Ptr cmd_paste( IGlyph* pGlyph, float x, float y, float qX, float qY );
    virtual IInteraction::Ptr cmd_paste( const std::set< IGlyph* >& selection, float x, float y, float qX, float qY );
    virtual void cmd_rotateLeft( const std::set< IGlyph* >& selection );
    virtual void cmd_rotateRight( const std::set< IGlyph* >& selection );
    virtual void cmd_flipHorizontally( const std::set< IGlyph* >& selection );
    virtual void cmd_flipVertically( const std::set< IGlyph* >& selection );
    
    void cmd_deleteProperties( const Node::PtrCstVector& nodes );
    void cmd_addProperties( const Node::PtrCstVector& nodes, const std::string& strName, const std::string& strStatement );
    void cmd_editProperties( const Node::PtrCstVector& nodes, const std::string& strName, const std::string& strStatement );

    
    Site::Ptr getSite() const { return m_pSite; }
    
    std::set< IGlyph* > generateExtrusion( float fAmount, bool bConvexHull );
    void save( const std::set< IGlyph* >& selection, const std::string& strFilePath );
    
protected:
    IInteraction::Ptr cmd_paste( Site::PtrVector sites, float x, float y, float qX, float qY );

    EditMain& m_editMain;
    GlyphFactory& m_glyphFactory;
    Site::Ptr m_pSite;
    IInteraction* m_pActiveInteraction;

    typedef std::map< Site::Ptr, boost::shared_ptr< EditNested > > SiteMap;
    SiteMap m_glyphMap;
};

}

#endif BASE_EDIT_17_DEC_2020/