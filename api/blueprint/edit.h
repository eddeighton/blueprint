#ifndef EDIT_14_09_2013
#define EDIT_14_09_2013

#include <vector>
#include <set>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>

//#include "common/stl.hpp"

#include "buffer.h"
#include "site.h"
#include "glyph.h"

namespace Blueprint
{

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class IEditContext
{
public:
    enum ToolType
    {
        eSelect =   100000,
        eDraw =     100001
    };
    virtual ~IEditContext(){}
    virtual IInteraction::Ptr interaction_start( float x, float y, float qX, float qY, IGlyph* pHitGlyph, const std::set< IGlyph* >& selection ) = 0;
    virtual IInteraction::Ptr interaction_draw( float x, float y, float qX, float qY, Site::Ptr pSite ) = 0;
    virtual IInteraction::Ptr interaction_tool( float x, float y, float qX, float qY, IGlyph* pHitGlyph, const std::set< IGlyph* >& selection, unsigned int uiToolID ) = 0;
    virtual IInteraction::Ptr interaction_tool_draw( float x, float y, float qX, float qY, Site::Ptr pSite, unsigned int uiToolID ) = 0;
    virtual IEditContext* getNestedContext( const std::vector< IGlyph* >& candidates ) = 0;
    virtual IEditContext* getParent() = 0;
    virtual const GlyphSpec* getImageSpec() const = 0;
    virtual bool canEdit( IGlyph* pGlyph, unsigned int uiToolType ) const = 0;
    
    virtual void cmd_delete( const std::set< IGlyph* >& selection ) = 0;
    virtual Node::Ptr cmd_cut( const std::set< IGlyph* >& selection ) = 0;
    virtual Node::Ptr cmd_copy( const std::set< IGlyph* >& selection ) = 0;
    virtual IInteraction::Ptr cmd_paste( Node::Ptr pPaste, float x, float y, float qX, float qY ) = 0;
    virtual IInteraction::Ptr cmd_paste( IGlyph* pGlyph, float x, float y, float qX, float qY ) = 0;
    virtual IInteraction::Ptr cmd_paste( const std::set< IGlyph* >& selection, float x, float y, float qX, float qY ) = 0;
    virtual void cmd_undo()=0;
    virtual void cmd_redo()=0;

    virtual void getCmds( CmdTarget::CmdInfo::List& cmds ) const = 0;
    virtual void getTools( CmdTarget::ToolInfo::List& tools ) const = 0;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class SpaceGlyphs;

class Edit : public IEditContext
{
public:
    typedef boost::shared_ptr< Edit > Ptr;

protected:
    Edit( GlyphFactory& glyphFactory, Site::Ptr pSite, const std::string& strFilePath = std::string() );
public:
    static Edit::Ptr create( GlyphFactory& glyphFactory, Site::Ptr pSite, const std::string& strFilePath = std::string() );

    //IEditContext
    virtual IInteraction::Ptr interaction_start( float x, float y, float qX, float qY, IGlyph* pHitGlyph, const std::set< IGlyph* >& selection );
    virtual IInteraction::Ptr interaction_draw( float x, float y, float qX, float qY, Site::Ptr pSite );
    virtual IInteraction::Ptr interaction_tool( float x, float y, float qX, float qY, IGlyph* pHitGlyph, const std::set< IGlyph* >& selection, unsigned int uiToolID );
    virtual IInteraction::Ptr interaction_tool_draw( float x, float y, float qX, float qY, Site::Ptr pSite, unsigned int uiToolID );

    virtual IEditContext* getNestedContext( const std::vector< IGlyph* >& candidates );
    virtual IEditContext* getParent() { return 0u; }
    virtual bool canEdit( IGlyph* pGlyph, unsigned int uiToolType ) const;
    virtual const GlyphSpec* getImageSpec() const { return 0u; }
    
    void interaction_evaluate();
    virtual void interaction_update();
    void interaction_end( IInteraction* pInteraction );

    virtual GlyphSpecProducer* fromGlyph( IGlyph* pGlyph ) const;

    //command handling
    virtual void cmd_delete( const std::set< IGlyph* >& selection );
    virtual Node::Ptr cmd_cut( const std::set< IGlyph* >& selection );
    virtual Node::Ptr cmd_copy( const std::set< IGlyph* >& selection );
    virtual IInteraction::Ptr cmd_paste( Node::Ptr pPaste, float x, float y, float qX, float qY );
    virtual IInteraction::Ptr cmd_paste( IGlyph* pGlyph, float x, float y, float qX, float qY );
    virtual IInteraction::Ptr cmd_paste( const std::set< IGlyph* >& selection, float x, float y, float qX, float qY );
    virtual void cmd_undo();
    virtual void cmd_redo();

    const std::string getFilePath() const { return m_strFilePath; }
    void setFilePath( const std::string& strFilePath ) { m_strFilePath = strFilePath; }
    
    virtual void getCmds( CmdTarget::CmdInfo::List& cmds ) const;
    virtual void getTools( CmdTarget::ToolInfo::List& tools ) const;
protected:
    IInteraction::Ptr cmd_paste( Site::PtrVector sites, float x, float y, float qX, float qY );

    GlyphFactory& m_glyphFactory;
    Site::Ptr m_pSite;
    IInteraction* m_pActiveInteraction;
    std::string m_strFilePath;

    Site::PtrList m_history;
    typedef std::map< Site::Ptr, boost::shared_ptr< SpaceGlyphs > > SiteMap;
    SiteMap m_glyphMap;

};

}


#endif //EDIT_14_09_2013