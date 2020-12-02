#ifndef EDIT_14_09_2013
#define EDIT_14_09_2013

#include <vector>
#include <set>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "buffer.h"
#include "site.h"
#include "glyph.h"

namespace Blueprint
{
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class IInteraction : public boost::enable_shared_from_this< IInteraction >
{
public:
    typedef boost::shared_ptr< IInteraction > Ptr;
    virtual ~IInteraction(){}
    virtual void OnMove( float x, float y ) = 0;
    virtual boost::shared_ptr< Site > GetInteractionSite() const = 0;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class IEditContext
{
public:
    enum ToolType
    {
        eSelect,
        eDraw
    };
    
    enum ToolMode
    {
        eArea,
        eContour
    };
    
    virtual ~IEditContext(){}
    virtual void activated() = 0;
    virtual IInteraction::Ptr interaction_start( ToolMode toolMode, float x, float y, float qX, float qY, IGlyph* pHitGlyph, const std::set< IGlyph* >& selection ) = 0;
    virtual IInteraction::Ptr interaction_draw( ToolMode toolMode, float x, float y, float qX, float qY, Site::Ptr pSite ) = 0;
    virtual IEditContext* getNestedContext( const std::vector< IGlyph* >& candidates ) = 0;
    virtual IEditContext* getParent() = 0;
    virtual bool isSiteContext( Site::Ptr pSite ) const = 0;
    virtual IEditContext* getSiteContext( Site::Ptr pSite ) = 0;
    virtual const Origin* getOrigin() const = 0;
    virtual bool canEdit( IGlyph* pGlyph, ToolType toolType, ToolMode toolMode ) const = 0;
    
    virtual void cmd_delete( const std::set< IGlyph* >& selection ) = 0;
    virtual Node::Ptr cmd_cut( const std::set< IGlyph* >& selection ) = 0;
    virtual Node::Ptr cmd_copy( const std::set< IGlyph* >& selection ) = 0;
    virtual IInteraction::Ptr cmd_paste( Node::Ptr pPaste, float x, float y, float qX, float qY ) = 0;
    virtual IInteraction::Ptr cmd_paste( IGlyph* pGlyph, float x, float y, float qX, float qY ) = 0;
    virtual IInteraction::Ptr cmd_paste( const std::set< IGlyph* >& selection, float x, float y, float qX, float qY ) = 0;
    virtual void cmd_rotateLeft( const std::set< IGlyph* >& selection ) = 0;
    virtual void cmd_rotateRight( const std::set< IGlyph* >& selection ) = 0;
    virtual void cmd_flipHorizontally( const std::set< IGlyph* >& selection ) = 0;
    virtual void cmd_flipVertically( const std::set< IGlyph* >& selection ) = 0;
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
    void interaction_end( IInteraction* pInteraction );

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

    const std::string getFilePath() const { return m_strFilePath; }
    void setFilePath( const std::string& strFilePath ) { m_strFilePath = strFilePath; }
    
    Site::Ptr getSite() const { return m_pSite; }
    
    std::set< IGlyph* > generateExtrusion( float fAmount, bool bConvexHull );
    void save( const std::set< IGlyph* >& selection, const std::string& strFilePath );
    
    void setViewMode( bool bArrangement, bool bCellComplex, bool bClearance );
protected:
    IInteraction::Ptr cmd_paste( Site::PtrVector sites, float x, float y, float qX, float qY );

    GlyphFactory& m_glyphFactory;
    Site::Ptr m_pSite;
    IInteraction* m_pActiveInteraction;
    std::string m_strFilePath;

    Site::PtrList m_history;
    typedef std::map< Site::Ptr, boost::shared_ptr< SpaceGlyphs > > SiteMap;
    SiteMap m_glyphMap;

    static bool m_bViewArrangement, m_bViewCellComplex, m_bViewClearance;
};

}


#endif //EDIT_14_09_2013