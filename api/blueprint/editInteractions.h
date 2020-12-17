
#ifndef INTERACTIONS_EDIT_17_DEC_202
#define INTERACTIONS_EDIT_17_DEC_202

#include "blueprint/buffer.h"
#include "blueprint/site.h"
#include "blueprint/glyph.h"

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <vector>
#include <set>

namespace Blueprint
{
    
class EditBase;

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
    virtual void interaction_end( IInteraction* pInteraction ) = 0;
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
class Interaction : public IInteraction
{
    friend class EditBase;
public:
    struct InitialValue
    {
        float x,y;
        InitialValue( float _x, float _y ) : x( _x ), y( _y ) {}
    };
    typedef std::vector< InitialValue > InitialValueVector;

private:
    Interaction( EditBase& edit, IEditContext::ToolMode toolMode, 
        float x, float y, float qX, float qY, 
        IGlyph::Ptr pHitGlyph, const IGlyph::PtrSet& selection );
    
public:
    virtual void OnMove( float x, float y );
    virtual Site::Ptr GetInteractionSite() const;

private:
    EditBase& m_edit;
    const IEditContext::ToolMode m_toolMode;
    float m_startX, m_startY, m_qX, m_qY;
    InitialValueVector m_initialValues;
    IGlyph::Ptr m_pHitGlyph;
    IGlyph::PtrVector m_interacted;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class InteractionToolWrapper : public IInteraction
{
    friend class EditBase;

private:
    InteractionToolWrapper( EditBase& edit, IInteraction::Ptr pWrapped );
    
public:
    virtual void OnMove( float x, float y );
    virtual Site::Ptr GetInteractionSite() const;

private:
    EditBase& m_edit;
    IInteraction::Ptr m_pToolInteraction;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Polygon_Interaction : public IInteraction
{
    friend class EditBase;
private:
    Polygon_Interaction( Site& site, float x, float y, float qX, float qY );
    
public:
    virtual void OnMove( float x, float y );
    virtual Site::Ptr GetInteractionSite() const;

private:
    Site& m_site;
    Polygon2D m_originalPolygon;
    float m_startX, m_startY, m_qX, m_qY;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
inline IInteraction::Ptr make_interaction_ptr( IEditContext* pEdit, IInteraction* pInteraction )
{
    return IInteraction::Ptr( pInteraction, [ pEdit ]( IInteraction* p ){ pEdit->interaction_end( p ); } );
}


}

#endif //INTERACTIONS_EDIT_17_DEC_202