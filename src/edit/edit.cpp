#include "blueprint/edit.h"

#include "spaces/basicarea.h"
#include "spaces/blueprint.h"

#include "blueprint/property.h"
#include "blueprint/dataBitmap.h"

#include "common/assert_verify.hpp"
#include "common/rounding.hpp"
//#include "common/stl.h"
//#include "common/log.h"

#include <sstream>
#include <map>
#include <iomanip>

namespace Blueprint
{
    
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Interaction : public IInteraction
{
    friend class Edit;
public:
    struct InitialValue
    {
        float x,y;
        InitialValue( float _x, float _y ) : x( _x ), y( _y ) {}
    };
    typedef std::vector< InitialValue > InitialValueVector;

private:
    Interaction( Edit& edit, IEditContext::ToolMode toolMode, 
        float x, float y, float qX, float qY, 
        IGlyph::Ptr pHitGlyph, const IGlyph::PtrSet& selection );
    
public:
    virtual void OnMove( float x, float y );
    virtual Site::Ptr GetInteractionSite() const
    {
        return m_edit.getSite();
    }

private:
    Edit& m_edit;
    const IEditContext::ToolMode m_toolMode;
    float m_startX, m_startY, m_qX, m_qY;
    InitialValueVector m_initialValues;
    IGlyph::Ptr m_pHitGlyph;
    IGlyph::PtrVector m_interacted;
};
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Interaction::Interaction( Edit& edit, IEditContext::ToolMode toolMode, 
                float x, float y, float qX, float qY, 
                IGlyph::Ptr pHitGlyph, const IGlyph::PtrSet& totalSelection )
    :   m_edit( edit ),
        m_toolMode( toolMode ),
        m_startX( x ),
        m_startY( y ),
        m_qX( qX ),
        m_qY( qY ),
        m_pHitGlyph( pHitGlyph )
{
    IGlyph::PtrSet selection;
    for( IGlyph::PtrSet::const_iterator i = totalSelection.begin(),
        iEnd = totalSelection.end(); i!=iEnd; ++i )
    {
        IGlyph::Ptr pGlyph = *i;
        //determine whether the glyph belongs to the current edit context...
        if( m_edit.canEdit( pGlyph.get(), IEditContext::eSelect, m_toolMode ) )
            selection.insert( pGlyph );
    }
    if( pHitGlyph && m_edit.canEdit( pHitGlyph.get(), IEditContext::eSelect, m_toolMode ) )
        selection.insert( pHitGlyph );

    if( !selection.empty() )
    {
        for( IGlyph::PtrSet::iterator i = selection.begin(),
            iEnd = selection.end(); i!=iEnd; ++i )
        {
            if( GlyphImage* p = dynamic_cast< GlyphImage* >( i->get() ) )
            {
                if( m_edit.canEdit( p, IEditContext::eSelect, m_toolMode ) )
                {
                    m_interacted.push_back( *i );
                    m_initialValues.push_back( InitialValue( p->getImageSpec()->getX(), p->getImageSpec()->getY() ) );
                }
            }
        }
        for( IGlyph::PtrSet::iterator i = selection.begin(),
            iEnd = selection.end(); i!=iEnd; ++i )
        {
            if( GlyphControlPoint* p = dynamic_cast< GlyphControlPoint* >( i->get() ) )
            {
                if( m_edit.canEdit( p, IEditContext::eSelect, m_toolMode ) )
                {
                    m_interacted.push_back( *i );
                    m_initialValues.push_back( InitialValue( p->getControlPoint()->getX(), p->getControlPoint()->getY() ) );
                }
            }
        }
    }
}

void Interaction::OnMove( float x, float y )
{
    float fDeltaX = Math::quantize_roundUp( x - m_startX, m_qX );
    float fDeltaY = Math::quantize_roundUp( y - m_startY, m_qY );

    InitialValueVector::iterator iValue = m_initialValues.begin();
    for( IGlyph::PtrVector::iterator i = m_interacted.begin(),
        iEnd = m_interacted.end(); i!=iEnd; ++i, ++iValue )
    {
        if( const GlyphSpecInteractive* p = 
            dynamic_cast< const GlyphSpecInteractive* >( i->get()->getGlyphSpec() ) )
        {
            const_cast< GlyphSpecInteractive* >( p )->set( 
                iValue->x + fDeltaX, iValue->y + fDeltaY );
        }
    }
    m_edit.interaction_evaluate();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class InteractionToolWrapper : public IInteraction
{
    friend class Edit;

private:
    InteractionToolWrapper( Edit& edit, IInteraction::Ptr pWrapped )
        :   m_edit( edit ),
            m_pToolInteraction( pWrapped )
    {
    }
    
public:
    virtual void OnMove( float x, float y )
    {
        m_pToolInteraction->OnMove( x, y );
        m_edit.interaction_evaluate();
    }
    virtual Site::Ptr GetInteractionSite() const
    {
        return m_edit.getSite();
    }

private:
    Edit& m_edit;
    IInteraction::Ptr m_pToolInteraction;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Boundary_Interaction : public IInteraction
{
    friend class Edit;
private:
    Boundary_Interaction( Area& area, float x, float y, float qX, float qY )
        :   m_area( area ),
            m_qX( qX ),
            m_qY( qY ),
            m_pBoundaryPoint( new Feature_ContourSegment( area.m_pContour, area.m_pContour->generateNewNodeName( "boundary" ) ) )
    {
        m_startX = x = Math::quantize_roundUp( x, qX );
        m_startY = y = Math::quantize_roundUp( y, qY );
        VERIFY_RTE( area.m_pContour->add( m_pBoundaryPoint ) );
        m_area.m_boundaryPoints.push_back( m_pBoundaryPoint );
        
        const wykobi::point2d< float >& origin = m_area.m_pContour->get()[0];
        OnMove( x, y );
    }
    
public:
    virtual void OnMove( float x, float y )
    {
        float fDeltaX = Math::quantize_roundUp( x, m_qX );
        float fDeltaY = Math::quantize_roundUp( y, m_qY );

        const wykobi::point2d< float >& origin = m_area.m_pContour->get()[0];
        m_pBoundaryPoint->set( 0, fDeltaX - origin.x, fDeltaY - origin.y );
    }
    virtual Site::Ptr GetInteractionSite() const
    {
        return m_area.shared_from_this();
    }

private:
    Area& m_area;
    Feature_ContourSegment::Ptr m_pBoundaryPoint;
    float m_startX, m_startY, m_qX, m_qY;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Polygon_Interaction : public IInteraction
{
    friend class Edit;
private:
    Polygon_Interaction( Area& area, float x, float y, float qX, float qY )
        :   m_area( area ),
            m_qX( qX ),
            m_qY( qY )
    {
        m_startX = x = Math::quantize_roundUp( x, qX );
        m_startY = y = Math::quantize_roundUp( y, qY );

        Feature_Contour::Ptr pContour = area.m_pContour;

        wykobi::polygon< float, 2 > poly = pContour->get();
        m_iPointIndex = poly.size();
        poly.push_back( wykobi::make_point< float >( m_startX, m_startY ) );
        area.m_pContour->set( poly );

        OnMove( x, y );
    }
    
public:
    virtual void OnMove( float x, float y )
    {
        const float fDeltaX = Math::quantize_roundUp( x, m_qX );
        const float fDeltaY = Math::quantize_roundUp( y, m_qY );

        m_area.m_pContour->set( m_iPointIndex, fDeltaX, fDeltaY  );
    }
    virtual Site::Ptr GetInteractionSite() const
    {
        return m_area.shared_from_this();
    }

private:
    Area& m_area;
    float m_startX, m_startY, m_qX, m_qY;
    int m_iPointIndex;
};


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class SpaceGlyphs : public Edit
{
public:
    typedef boost::shared_ptr< SpaceGlyphs > Ptr;
    typedef std::map< Site::Ptr, SpaceGlyphs::Ptr > Map;
    typedef std::map< Feature::Ptr, IGlyph::PtrSet > FeatureGlyphMap;
    typedef std::map< const GlyphSpec*, IGlyph::Ptr > GlyphMap;

    SpaceGlyphs( IEditContext& parentContext, Site::Ptr pSpace, GlyphFactory& glyphFactory );
    virtual void interaction_update();

    virtual IEditContext* getParent() { return &m_parent; }
    virtual const GlyphSpec* getImageSpec() const { return m_pSite.get(); }

    void matchFeatures();
    bool owns( IGlyph* pGlyph ) const;
    bool owns( const GlyphSpec* pGlyphSpec ) const;
    IGlyph::Ptr getMainGlyph() const { return m_pImageGlyph; }
    virtual void cmd_delete( const std::set< IGlyph* >& selection );
    
    virtual GlyphSpecProducer* fromGlyph( IGlyph* pGlyph ) const;
private:
    IEditContext& m_parent;
    Site::Ptr m_pSite;
    IGlyph::Ptr m_pImageGlyph;
    FeatureGlyphMap m_features;
    GlyphMap m_glyphs;
    GlyphMap m_markup;
};

GlyphSpecProducer* SpaceGlyphs::fromGlyph( IGlyph* pGlyph ) const
{
    GlyphSpecProducer* pResult = Edit::fromGlyph( pGlyph );
    if( !pResult )
    {
        IGlyph::Ptr pGlyphPtr = pGlyph->shared_from_this();
        for( auto i = m_features.begin(),
            iEnd = m_features.end(); i!=iEnd; ++i )
        {
            if( i->second.find( pGlyphPtr ) != i->second.end() )
            {
                pResult = i->first.get();
                break;
            }
        }
    }

    return pResult;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Edit::Edit( GlyphFactory& glyphFactory, Site::Ptr pSite, const std::string& strFilePath )
    :   m_glyphFactory( glyphFactory ),
        m_pSite( pSite ),
        m_pActiveInteraction( 0u ),
        m_strFilePath( strFilePath )
{
}

GlyphSpecProducer* Edit::fromGlyph( IGlyph* pGlyph ) const
{
    GlyphSpecProducer* pResult = nullptr;

    for( auto i = m_glyphMap.begin(),
        iEnd = m_glyphMap.end(); i!=iEnd; ++i )
    {
        if( i->second->getMainGlyph().get() == pGlyph )
        {
            pResult = i->first.get();
            break;
        }
    }

    return pResult;
}

Edit::Ptr Edit::create( GlyphFactory& glyphFactory, Site::Ptr pSite, const std::string& strFilePath )
{
    Edit::Ptr pNewEdit;

    Blueprint::Ptr pBlueprint;

    if( Clip::Ptr pClip = boost::dynamic_pointer_cast< Clip >( pSite ) )
    {
        pBlueprint.reset( new Blueprint( pClip->Node::getName() ) );
        for( Node::PtrVector::const_iterator 
            i = pClip->Node::getChildren().begin(),
            iEnd = pClip->Node::getChildren().end(); i!=iEnd; ++i )
            pBlueprint->add( (*i)->copy( pBlueprint, (*i)->Node::getName() ) );

        pNewEdit.reset( new Edit( glyphFactory, pBlueprint, strFilePath ) );
        pNewEdit->interaction_evaluate();
    }
    else if( pBlueprint = boost::dynamic_pointer_cast< Blueprint >( pSite ) )
    {
        pNewEdit.reset( new Edit( glyphFactory, pSite, strFilePath ) );
        pNewEdit->interaction_evaluate();
    }
    else
    {
        pBlueprint.reset( new Blueprint( pSite->Node::getName() ) );
        pBlueprint->add( pSite->copy( pBlueprint, pSite->Node::getName() ) );
    }
    
    VERIFY_RTE( pBlueprint );

    pNewEdit.reset( new Edit( glyphFactory, pBlueprint, strFilePath ) );
    pNewEdit->interaction_evaluate();

    return pNewEdit;
}

IInteraction::Ptr Edit::interaction_start( ToolMode toolMode, float x, float y, float qX, float qY, IGlyph* pHitGlyph, const std::set< IGlyph* >& selection )
{
    ASSERT( !m_pActiveInteraction );
    
    IGlyph::PtrSet selectionShared;
    for( std::set< IGlyph* >::const_iterator i = selection.begin(),
        iEnd = selection.end(); i!=iEnd; ++i )
    {
        IGlyph* pGlyph = *i;
        selectionShared.insert( pGlyph->shared_from_this() );
    }
    IGlyph::Ptr pHit = pHitGlyph ? pHitGlyph->shared_from_this() : IGlyph::Ptr();
    m_pActiveInteraction = new Interaction( *this, toolMode, x, y, qX, qY, pHit, selectionShared );
    return IInteraction::Ptr( m_pActiveInteraction, boost::bind( &Edit::interaction_end, this, _1 ) );
}

IInteraction::Ptr Edit::interaction_draw( ToolMode toolMode, float x, float y, float qX, float qY, Site::Ptr pSite )
{
    ASSERT( !m_pActiveInteraction );
    
    if( toolMode == IEditContext::eContour )
    {
        x = Math::quantize_roundUp( x , qX );
        y = Math::quantize_roundUp( y , qY );
        
        if( Area* pArea = dynamic_cast< Area* >( m_pSite.get() ) )
        {
            IInteraction::Ptr pToolInteraction( new Polygon_Interaction( *pArea, x, y, qX, qY ) );
            interaction_evaluate();
            m_pActiveInteraction = new InteractionToolWrapper( *this, pToolInteraction );
            return IInteraction::Ptr( m_pActiveInteraction, boost::bind( &Edit::interaction_end, this, _1 ) );
        }
        else
        {
            //create new area
            Area::Ptr pNewSite( new Area( m_pSite, m_pSite->generateNewNodeName( "area" ) ) );
            pNewSite->init( x, y, true );
            m_pSite->add( pNewSite );
            interaction_evaluate();
            
            Edit* pNewEdit = dynamic_cast< Edit* >( getSiteContext( pNewSite ) );
            ASSERT( pNewEdit );

            IInteraction::Ptr pToolInteraction( new Polygon_Interaction( *pNewSite, 0, 0, qX, qY ) );
            pNewEdit->m_pActiveInteraction = new InteractionToolWrapper( *pNewEdit, pToolInteraction );
            pNewEdit->interaction_evaluate();
            return IInteraction::Ptr( pNewEdit->m_pActiveInteraction, boost::bind( &Edit::interaction_end, pNewEdit, _1 ) );
        }
    }
    else if( toolMode == IEditContext::eConnection )
    {
        x = Math::quantize_roundUp( x , qX );
        y = Math::quantize_roundUp( y , qY );
        
        if( Area* pArea = dynamic_cast< Area* >( m_pSite.get() ) )
        {
            IInteraction::Ptr pToolInteraction( new Boundary_Interaction( *pArea, x, y, qX, qY ) );
            interaction_evaluate();
            m_pActiveInteraction = new InteractionToolWrapper( *this, pToolInteraction );
            return IInteraction::Ptr( m_pActiveInteraction, boost::bind( &Edit::interaction_end, this, _1 ) );
        }
    }
    else if( toolMode == IEditContext::eArea )
    {
        x = Math::quantize_roundUp( x , qX );
        y = Math::quantize_roundUp( y , qY );

        //generate space name    
        Site::PtrSet newSites;
        if( boost::dynamic_pointer_cast< Clip >( pSite ) || 
            boost::dynamic_pointer_cast< Blueprint >( pSite ) )
        {
            //determine the centroid of the clip and offset accordingly
            std::vector< wykobi::point2d< float > > points;
            wykobi::point2d< float > ptAverage = wykobi::make_point( 0.0f, 0.0f );
            for( Node::PtrVector::const_iterator 
                i = pSite->getChildren().begin(),
                iEnd = pSite->getChildren().end(); i!=iEnd; ++i )
            {
                if( Site::Ptr pClipSite = boost::dynamic_pointer_cast< Site >( *i ) )
                {
                    Site::Ptr pClipSiteCopy = 
                        boost::dynamic_pointer_cast< Site >( pClipSite->copy( m_pSite, 
                            m_pSite->generateNewNodeName( "area" ) ) );
                    m_pSite->add( pClipSiteCopy );
                    newSites.insert( pClipSiteCopy );
                    ptAverage.x += pClipSiteCopy->getX();
                    ptAverage.y += pClipSiteCopy->getY();
                } 
            }
            ptAverage.x /= static_cast< float >( newSites.size() );
            ptAverage.y /= static_cast< float >( newSites.size() );
            ptAverage.x = Math::quantize_roundUp( ptAverage.x , qX );
            ptAverage.y = Math::quantize_roundUp( ptAverage.y , qY );
            for( Site::PtrSet::iterator i = newSites.begin(),
                iEnd = newSites.end(); i!=iEnd; ++i )
            {
                Site::Ptr pClipSiteCopy = *i;
                pClipSiteCopy->set( 
                    pClipSiteCopy->getX() + x - ptAverage.x, 
                    pClipSiteCopy->getY() + y - ptAverage.y );
            }
        }
        else if( Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( pSite ) )
        {
            Area::Ptr pNewSite = 
                boost::dynamic_pointer_cast< Area >( pArea->copy( m_pSite, 
                    m_pSite->generateNewNodeName( "area" ) ) );
            pNewSite->init( x, y, false );
            m_pSite->add( pNewSite );
            newSites.insert( pNewSite );
        }

        m_pSite->init();

        interaction_evaluate();
        
        IGlyph::PtrSet selection;
        for( Site::PtrSet::iterator i = newSites.begin(),
            iEnd = newSites.end(); i!=iEnd; ++i )
        {
            SiteMap::iterator iFind = m_glyphMap.find( *i );
            if( iFind != m_glyphMap.end() )
                selection.insert( iFind->second->getMainGlyph() );
        }

        m_pActiveInteraction = new Interaction( *this, toolMode, x, y, qX, qY, IGlyph::Ptr(), selection );
        m_pActiveInteraction->OnMove( x, y );
        return IInteraction::Ptr( m_pActiveInteraction, boost::bind( &Edit::interaction_end, this, _1 ) );
            
    }
    else
    {
        THROW_RTE( "Unknown tool mode" );
    }
}
/*
IInteraction::Ptr Edit::interaction_tool( float x, float y, float qX, float qY, IGlyph* pHitGlyph, const std::set< IGlyph* >& selection, unsigned int uiToolID )
{
    IInteraction::Ptr pNewInteraction;

    GlyphSpecProducer* pHit = fromGlyph( pHitGlyph );
    std::set< GlyphSpecProducer* > nodeSelection;
    for( auto i = selection.begin(),
        iEnd = selection.end(); i!=iEnd; ++i )
    {
        if( GlyphSpecProducer* pSel = fromGlyph( *i ) )
            nodeSelection.insert( pSel );
    }
    
    if( IInteraction::Ptr pToolInteraction = m_pSite->beginTool( uiToolID, x, y, qX, qY, pHit, nodeSelection ) )
    {
        interaction_evaluate();
        m_pActiveInteraction = new InteractionToolWrapper( *this, pToolInteraction );
        return IInteraction::Ptr( m_pActiveInteraction, boost::bind( &Edit::interaction_end, this, _1 ) );
    }
    else
    {
        ASSERT( false );
        return IInteraction::Ptr();
    }
}

IInteraction::Ptr Edit::interaction_tool_draw( float x, float y, float qX, float qY, Site::Ptr pSite, unsigned int uiToolID )
{
    ASSERT( !m_pActiveInteraction );
    
    if( IInteraction::Ptr pToolInteraction = m_pSite->beginToolDraw( uiToolID, x, y, qX, qY, pSite ) )
    {
        interaction_evaluate();
        m_pActiveInteraction = new InteractionToolWrapper( *this, pToolInteraction );
        return IInteraction::Ptr( m_pActiveInteraction, boost::bind( &Edit::interaction_end, this, _1 ) );
    }
    else
    {
        ASSERT( false );
        return IInteraction::Ptr();
    }
}
*/
IEditContext* Edit::getNestedContext( const std::vector< IGlyph* >& candidates )
{
    IEditContext* pEditContext = 0u;

    for( std::vector< IGlyph* >::const_iterator 
        i = candidates.begin(),
        iEnd = candidates.end(); i!=iEnd && !pEditContext; ++i )
    {
        for( SpaceGlyphs::Map::const_iterator 
            j = m_glyphMap.begin(),
            jEnd = m_glyphMap.end(); j!=jEnd; ++j )
        {
            if( j->second->owns( *i ) )
            {
                pEditContext = j->second.get();
                break;
            }
        }
    }

    return pEditContext;
}

IEditContext* Edit::getSiteContext( Site::Ptr pSite )
{
    SiteMap::const_iterator iFind = m_glyphMap.find( pSite );
    if( iFind != m_glyphMap.end() )
    {
        return iFind->second.get();
    }
    return nullptr;
}

bool Edit::canEdit( IGlyph* pGlyph, ToolType toolType, ToolMode toolMode ) const
{
    
    if( pGlyph->getGlyphSpec()->canEdit() )
    {
        
        if( const GlyphSpecProducer* pGlyphPrd = fromGlyph( pGlyph ) )
        {
            return true;
            /*if( dynamic_cast< const Feature_ContourSegment* >( pGlyphPrd ) )
            {
                
            }
            else if( dynamic_cast< const Site* >( pGlyphPrd ) )
            {
                
            }
            else
            {
                
            }*/
            
            
            
            //return m_pSite->canEditWithTool( pGlyphPrd, uiToolType );
        }
        
        
    }
    return false;
    
    
    /*
    bool bCanEdit = false;
    
    if( pGlyph->getGlyphSpec()->canEdit() )
    {
        for( SpaceGlyphs::Map::const_iterator 
            j = m_glyphMap.begin(),
            jEnd = m_glyphMap.end(); j!=jEnd; ++j )
        {
            if( j->second->owns( pGlyph ) )
            {
                bCanEdit = true;
                break;
            }
        }
    }

    return bCanEdit;*/
}


void Edit::interaction_evaluate()
{
    //LOG_PROFILE_BEGIN( edit_interaction_evaluate );

    DataBitmap data;
    m_pSite->evaluate( data );
    interaction_update();

    //LOG_PROFILE_END( edit_interaction_evaluate );
}

void Edit::interaction_update()
{
    Site::PtrSet sites( m_pSite->getSpaces().begin(), m_pSite->getSpaces().end() );

    SiteMap removals;
    Site::PtrVector additions;
    generics::match( m_glyphMap.begin(), m_glyphMap.end(), sites.begin(), sites.end(),
        generics::lessthan( generics::first< SiteMap::const_iterator >(), generics::deref< Site::PtrSet::const_iterator >() ),
        generics::collect( removals, generics::deref< SiteMap::const_iterator >() ),
        generics::collect( additions, generics::deref< Site::PtrSet::const_iterator >() ) );

    for( SiteMap::iterator 
        i = removals.begin(),
        iEnd = removals.end(); i!=iEnd; ++i )
        m_glyphMap.erase( i->first );

    for( Site::PtrVector::iterator 
        i = additions.begin(),
        iEnd = additions.end(); i!=iEnd; ++i )
        m_glyphMap.insert( std::make_pair( *i, SpaceGlyphs::Ptr( new SpaceGlyphs( *this, *i, m_glyphFactory ) ) ) );

    generics::for_each_second( m_glyphMap, boost::bind( &SpaceGlyphs::interaction_update, _1 ) );
}

void Edit::interaction_end( IInteraction* pInteraction )
{
    ASSERT( m_pActiveInteraction == pInteraction );
    delete m_pActiveInteraction;
    m_pActiveInteraction = 0u;
}

void Edit::cmd_delete( const std::set< IGlyph* >& selection )
{
    for( std::set< IGlyph* >::iterator 
        i = selection.begin(),
        iEnd = selection.end(); i!=iEnd; ++i )
    {
        IGlyph* pGlyph = *i;
        
        for( SiteMap::const_iterator j = m_glyphMap.begin(),
            jEnd = m_glyphMap.end(); j!=jEnd; ++j )
        {
            Site::Ptr pSite = j->first;
            if( dynamic_cast< const GlyphSpec* >( pSite.get() ) == pGlyph->getGlyphSpec() )
            {
                m_glyphMap.erase( j );
                m_pSite->remove( pSite );
                break;
            }
        }
    }
    
    m_pSite->init();

    interaction_evaluate();
}

Node::Ptr Edit::cmd_cut( const std::set< IGlyph* >& selection )
{
    Node::Ptr pResult;

    Site::PtrSet sites;
    for( std::set< IGlyph* >::const_iterator 
        i = selection.begin(), iEnd = selection.end(); i!=iEnd; ++i )
    {
        IGlyph* pGlyph = *i;

        for( SiteMap::const_iterator j = m_glyphMap.begin(),
            jEnd = m_glyphMap.end(); j!=jEnd; ++j )
        {
            Site::Ptr pSite = j->first;
            if( dynamic_cast< const GlyphSpec* >( pSite.get() ) == pGlyph->getGlyphSpec() )
            {
                m_glyphMap.erase( j );
                m_pSite->remove( pSite );
                sites.insert( pSite );
                break;
            }
        }
    }

    if( sites.size() == 1u )
    {
        pResult = *sites.begin();
    }
    else if( !sites.empty() )
    {
        Clip::Ptr pClip( new Clip( Node::Ptr(), "clip" ) );
        for( Site::PtrSet::iterator 
            i = sites.begin(),
            iEnd = sites.end(); i!=iEnd; ++i )
        {
            VERIFY_RTE( pClip->add( (*i)->copy( pClip, (*i)->Node::getName() ) ) );
        }
        pResult = pClip;
    }
    
    m_pSite->init();

    interaction_evaluate();

    return pResult;
}

Node::Ptr Edit::cmd_copy( const std::set< IGlyph* >& selection )
{
    Node::Ptr pResult;

    Site::PtrSet sites;
    for( std::set< IGlyph* >::const_iterator 
        i = selection.begin(), iEnd = selection.end(); i!=iEnd; ++i )
    {
        IGlyph* pGlyph = *i;

        for( SiteMap::const_iterator j = m_glyphMap.begin(),
            jEnd = m_glyphMap.end(); j!=jEnd; ++j )
        {
            Site::Ptr pSite = j->first;
            if( dynamic_cast< const GlyphSpec* >( pSite.get() ) == pGlyph->getGlyphSpec() )
            {
                sites.insert( pSite );
                break;
            }
        }
    }

    if( sites.size() == 1u )
    {
        pResult = (*sites.begin())->copy( Site::Node::Ptr(), "copy" );
    }
    else if( !sites.empty() )
    {
        Clip::Ptr pClip( new Clip( Node::Ptr(), "copy" ) );
        for( Site::PtrSet::iterator 
            i = sites.begin(),
            iEnd = sites.end(); i!=iEnd; ++i )
        {
            bool bResult = pClip->add( (*i)->copy( pClip, (*i)->Node::getName() ) );
            VERIFY_RTE( bResult );
        }
        pResult = pClip;
    }

    interaction_evaluate();

    return pResult;
}

IInteraction::Ptr Edit::cmd_paste( Node::Ptr pPaste, float x, float y, float qX, float qY )
{
    IInteraction::Ptr pNewInteraction;
    
    Site::PtrVector sites;
    if( Clip::Ptr pClip = 
        boost::dynamic_pointer_cast< Clip >( pPaste ) )
    {
        for( Node::PtrVector::const_iterator i = pClip->getChildren().begin(),
            iEnd = pClip->getChildren().end(); i!=iEnd; ++i )
        {
            if( Site::Ptr pSite = 
                boost::dynamic_pointer_cast< Site >( *i ) )
                sites.push_back( pSite );
        }
    }
    else if( Site::Ptr pSite = 
        boost::dynamic_pointer_cast< Site >( pPaste ) )
    {
        sites.push_back( pSite );
    }
    
    return cmd_paste( sites, x, y, qX, qY );
}

IInteraction::Ptr Edit::cmd_paste( IGlyph* pGlyph, float x, float y, float qX, float qY )
{
    Site::PtrVector sites;
    if( const Area* pSite = dynamic_cast< const Area* >( pGlyph->getGlyphSpec() ) )
        sites.push_back( const_cast< Area* >( pSite )->shared_from_this() );
    return cmd_paste( sites, x, y, qX, qY );
}
IInteraction::Ptr Edit::cmd_paste( const std::set< IGlyph* >& selection, float x, float y, float qX, float qY )
{
    Site::PtrVector sites;
    for( std::set< IGlyph* >::iterator 
        i = selection.begin(),
        iEnd = selection.end(); i!=iEnd; ++i )
    {
        if( const Area* pSite = dynamic_cast< const Area* >( (*i)->getGlyphSpec() ) )
            sites.push_back( const_cast< Area* >( pSite )->shared_from_this() );
    }
    return cmd_paste( sites, x, y, qX, qY );
}

IInteraction::Ptr Edit::cmd_paste( Site::PtrVector sites, float x, float y, float qX, float qY )
{
    ASSERT( !m_pActiveInteraction );

    IInteraction::Ptr pNewInteraction;

    IGlyph::PtrSet copies;
    for( Site::PtrVector::const_iterator 
        i = sites.begin(),
        iEnd = sites.end(); i!=iEnd; ++i )
    {
        const std::string strNewKey = m_pSite->generateNewNodeName( "area" );
        Site::Ptr pCopy = boost::dynamic_pointer_cast< Site >( 
            (*i)->copy( m_pSite, strNewKey ) );
        VERIFY_RTE( pCopy );
        m_pSite->add( pCopy );

        SpaceGlyphs::Ptr pSpaceGlyphs( new SpaceGlyphs( *this, pCopy, m_glyphFactory ) );
        m_glyphMap.insert( std::make_pair( pCopy, pSpaceGlyphs ) );

        copies.insert( pSpaceGlyphs->getMainGlyph() );
    }

    m_pActiveInteraction = new Interaction( *this, IEditContext::eArea, x, y, qX, qY, IGlyph::Ptr(), copies );
    pNewInteraction = IInteraction::Ptr( m_pActiveInteraction, boost::bind( &Edit::interaction_end, this, _1 ) );
    
    m_pSite->init();

    interaction_evaluate();

    return pNewInteraction;
}
    
void Edit::cmd_undo()
{
    interaction_evaluate();
}

void Edit::cmd_redo()
{
    interaction_evaluate();
}

void Edit::getCmds( Site::CmdInfo::List& cmds ) const
{
    m_pSite->getCmds( cmds );
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
SpaceGlyphs::SpaceGlyphs( IEditContext& parentContext, Site::Ptr pSpace, GlyphFactory& glyphFactory )
    :   Edit( glyphFactory, pSpace ),
        m_parent( parentContext ),
        m_pSite( pSpace )
{
    //create the main image glyph
    if( SpaceGlyphs* pParentGlyphs = dynamic_cast< SpaceGlyphs* >( &m_parent ) )
        m_pImageGlyph = m_glyphFactory.createImage( m_pSite.get(), pParentGlyphs->getMainGlyph() );
    else
        m_pImageGlyph = m_glyphFactory.createImage( m_pSite.get(), IGlyph::Ptr() );

    MarkupText::List texts;
    m_pSite->getMarkupTexts( texts );
    for( MarkupText::List::const_iterator i = texts.begin(),
        iEnd = texts.end(); i!=iEnd; ++i )
    {
        if( IGlyph::Ptr pMarkupGlyph = m_glyphFactory.createMarkupText( *i, m_pImageGlyph ) )
            m_markup.insert( std::make_pair( *i, pMarkupGlyph ) );
    }

    MarkupPath::List paths;
    m_pSite->getMarkupPaths( paths );
    for( MarkupPath::List::const_iterator i = paths.begin(),
        iEnd = paths.end(); i!=iEnd; ++i )
    {
        if( IGlyph::Ptr pMarkupGlyph = m_glyphFactory.createMarkupPath( *i, m_pImageGlyph ) )
            m_markup.insert( std::make_pair( *i, pMarkupGlyph ) );
    }
    
}

typedef std::pair< Feature::Ptr, ControlPoint* > AddGlyphTask;
typedef std::list< AddGlyphTask > AddGlyphTaskList;

//inline bool needUpdate( const IGlyph::PtrSet& glyphSet, Feature::Ptr pFeature )
inline bool needUpdate( std::map< Feature::Ptr, IGlyph::PtrSet >::const_iterator i, Feature::PtrSet::const_iterator j )
{
    //typedef std::map< Feature::Ptr, IGlyph::PtrSet > FeatureGlyphMap;
    bool bNeedUpdate = false;
    if( (*j)->getControlPointCount() != i->second.size() )
        bNeedUpdate = true;
    //if( pFeature->getControlPointCount() != glyphSet.size() )
    //    bNeedUpdate = true;
    return bNeedUpdate;
}

void SpaceGlyphs::matchFeatures()
{
    Feature::PtrSet features;
    m_pSite->for_each_recursive( 
        generics::collectIfConvert( features, Node::ConvertPtrType< Feature >(), Node::ConvertPtrType< Feature >() ), 
        Node::ConvertPtrType< Site >() );

    /*
    Feature::PtrVector removals, additions;
    generics::match( m_features.begin(), m_features.end(), features.begin(), features.end(),
        generics::lessthan( generics::first< FeatureGlyphMap::const_iterator >(), 
                            generics::deref< Feature::PtrSet::const_iterator >() ),
          generics::collect( removals, generics::first< FeatureGlyphMap::const_iterator  >() ),
          generics::collect( additions, generics::deref< Feature::PtrSet::const_iterator  >() ) );
          */
    
    Feature::PtrVector removals, additions, updates;
    generics::matchGetUpdates( m_features.begin(), m_features.end(), features.begin(), features.end(),
        generics::lessthan( generics::first< FeatureGlyphMap::const_iterator >(), 
                            generics::deref< Feature::PtrSet::const_iterator >() ),
                            boost::bind( &needUpdate, _1, _2 ),
          generics::collect( removals, generics::first< FeatureGlyphMap::const_iterator  >() ),
          generics::collect( additions, generics::deref< Feature::PtrSet::const_iterator  >() ),
          generics::collect( updates, generics::first< FeatureGlyphMap::const_iterator  >() ) );
    
    for( Feature::PtrVector::iterator 
        i = updates.begin(),
        iEnd = updates.end(); i!=iEnd; ++i )
    {
        Feature::Ptr pFeature = *i;
        removals.push_back( pFeature );
        additions.push_back( pFeature );
    }

    for( Feature::PtrVector::iterator 
        i = removals.begin(),
        iEnd = removals.end(); i!=iEnd; ++i )
    {
        IGlyph::PtrSet& glyphs = m_features[ *i ];
        for( IGlyph::PtrSet::const_iterator j = glyphs.begin(),
            jEnd = glyphs.end(); j!=jEnd; ++j )
            m_glyphs.erase( (*j)->getGlyphSpec() );
        m_features.erase( *i );
    }

    AddGlyphTaskList tasks;
    for( Feature::PtrVector::iterator 
        i = additions.begin(),
        iEnd = additions.end(); i!=iEnd; ++i )
    {
        //get the glyph from the feature...
        Feature::Ptr pFeature = *i;

        //create the control points
        ControlPoint::List controlPoints;
        pFeature->getControlPoints( controlPoints );

        IGlyph::PtrSet glyphs;
        for( ControlPoint::List::iterator j = controlPoints.begin(),
            jEnd = controlPoints.end(); j!=jEnd; ++j )
            tasks.push_back( AddGlyphTask( pFeature, *j ) );
    }

    
    bool bContinue = true;
    while( bContinue )
    {
        bContinue = false;
        AddGlyphTaskList taskSwap;
        for( AddGlyphTaskList::iterator i = tasks.begin(),
            iEnd = tasks.end(); i!=iEnd; ++i )
        {
            const AddGlyphTask& task = *i;

            IGlyph::Ptr pParentGlyph;
            if( task.second->getParent() )
            {
                if( const ImageSpec* pParentImage = dynamic_cast< const ImageSpec* >( task.second->getParent() ) )
                {
                    ASSERT( m_pSite.get() == pParentImage );
                    pParentGlyph = m_pImageGlyph;
                }
                else
                {
                    GlyphMap::const_iterator iFind = m_glyphs.find( task.second->getParent() );
                    if( iFind != m_glyphs.end() )
                        pParentGlyph = iFind->second;
                }
            }

            //see if the glyph parent is created
            if( !task.second->getParent() || pParentGlyph)
            {
                IGlyph::Ptr pNewGlyph = m_glyphFactory.createControlPoint( task.second, pParentGlyph );
                m_glyphs.insert( std::make_pair( task.second, pNewGlyph ) );
                FeatureGlyphMap::iterator iFind = m_features.find( task.first );
                if( iFind == m_features.end() )
                {
                    IGlyph::PtrSet glyphs;
                    glyphs.insert( pNewGlyph );
                    m_features.insert( std::make_pair( task.first, glyphs ) );
                }
                else
                    iFind->second.insert( pNewGlyph );
                bContinue = true;
            }
            else
                taskSwap.push_back( *i );
        }
        tasks.swap( taskSwap );
    }
    VERIFY_RTE( tasks.empty() );
}

void SpaceGlyphs::interaction_update()
{
    //recurse
    Edit::interaction_update();

    matchFeatures();
    
    m_pImageGlyph->update();
    generics::for_each_second( m_glyphs, boost::bind( &IGlyph::update, _1 ) );
    generics::for_each_second( m_markup, boost::bind( &IGlyph::update, _1 ) );
}

template< class T >
bool containsSecondPtr( const T& cont, IGlyph* pGlyph )
{
    bool bOwnsGlyph = false;
    typedef typename T::const_iterator Iter;
    for( Iter i = cont.begin(),
        iEnd = cont.end(); i!=iEnd; ++i )
    {
        if( i->second.find( pGlyph->shared_from_this() ) != i->second.end() )
        {
            bOwnsGlyph = true;
            break;
        }
    }
    return bOwnsGlyph;
}

bool SpaceGlyphs::owns( const GlyphSpec* pGlyphSpec ) const
{
    return m_glyphs.find( pGlyphSpec ) != m_glyphs.end();
}

bool SpaceGlyphs::owns( IGlyph* pGlyph ) const
{
    return m_pImageGlyph.get() == pGlyph || 
        containsSecondPtr( m_features, pGlyph );
}
/*
bool SpaceGlyphs::canEdit( IGlyph* pGlyph, ToolType toolType, ToolMode toolMode ) const
{
    if( pGlyph->getGlyphSpec()->canEdit() )
    {
        if( const GlyphSpecProducer* pGlyphPrd = fromGlyph( pGlyph ) )
            return m_pSite->canEdit( pGlyphPrd, uiToolType );
    }
    return false;
}
*/

void SpaceGlyphs::cmd_delete( const std::set< IGlyph* >& selection )
{
    for( std::set< IGlyph* >::iterator 
        i = selection.begin(),
        iEnd = selection.end(); i!=iEnd; ++i )
    {
        IGlyph* pGlyph = *i;
        
        if( GlyphSpecProducer* pGlyphPrd = const_cast< GlyphSpecProducer* >( fromGlyph( pGlyph ) ) )
        {
            if( !dynamic_cast< Site* >( pGlyphPrd ) )
            {
                if( !pGlyphPrd->cmd_delete( pGlyph->getGlyphSpec() ) )
                {
                    if( Node::Ptr pParent = pGlyphPrd->getParent() )
                    {
                        pParent->remove( const_cast< GlyphSpecProducer* >( pGlyphPrd )->getPtr() );
                        break;
                    }
                }
            }
        }
    }
    
    Edit::cmd_delete( selection );
}
}