#include "blueprint/edit.h"

#include "blueprint/clip.h"
#include "blueprint/space.h"
#include "blueprint/blueprint.h"

#include "blueprint/property.h"
#include "blueprint/dataBitmap.h"
#include "blueprint/factory.h"
#include "blueprint/spaceUtils.h"

#include "common/assert_verify.hpp"
#include "common/rounding.hpp"

#include "wykobi_algorithm.hpp"

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
            if( GlyphOrigin* p = dynamic_cast< GlyphOrigin* >( i->get() ) )
            {
                if( m_edit.canEdit( p, IEditContext::eSelect, m_toolMode ) )
                {
                    m_interacted.push_back( *i );
                    m_initialValues.push_back( InitialValue( p->getOrigin()->getTransform().X(), p->getOrigin()->getTransform().Y() ) );
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
class Polygon_Interaction : public IInteraction
{
    friend class Edit;
private:
    Polygon_Interaction( Site& site, float x, float y, float qX, float qY )
        :   m_site( site ),
            m_qX( qX ),
            m_qY( qY )
    {
        m_startX = x = Math::quantize_roundUp( x, qX );
        m_startY = y = Math::quantize_roundUp( y, qY );

        if( Feature_Contour::Ptr pContour = m_site.getContour() )
        {
            m_originalPolygon = pContour->getPolygon();
            
            const Point2D pt = wykobi::make_point< float >( m_startX, m_startY );
            Polygon2D newPoly = m_originalPolygon;
            newPoly.push_back( pt );
            pContour->set( newPoly );
            
            OnMove( x, y );
        }
    }
    
public:
    virtual void OnMove( float x, float y )
    {
        if( Feature_Contour::Ptr pContour = m_site.getContour() )
        {
            const float fDeltaX = Math::quantize_roundUp( x, m_qX );
            const float fDeltaY = Math::quantize_roundUp( y, m_qY );
            const Point2D pt = wykobi::make_point< float >( fDeltaX, fDeltaY );
            const SegmentIDRatioPointTuple sirpt =
                findClosestPointOnContour( m_originalPolygon, pt );
            
            unsigned int uiIndex = std::get< 0 >( sirpt );
            VERIFY_RTE( uiIndex < m_originalPolygon.size() || uiIndex == 0 );
            if( m_originalPolygon.size() > 0 )
                uiIndex = ( uiIndex + 1 ) % m_originalPolygon.size();
            
            Polygon2D newPoly;
            for( std::size_t sz = 0U; ( sz != uiIndex ) && ( sz < m_originalPolygon.size() ); ++sz )
                newPoly.push_back( m_originalPolygon[ sz ] );
            newPoly.push_back( pt );
            for( std::size_t sz = uiIndex; sz < m_originalPolygon.size(); ++sz )
                newPoly.push_back( m_originalPolygon[ sz ] );
            
            if( wykobi::polygon_orientation( newPoly ) == wykobi::Clockwise )
                std::reverse( newPoly.begin(), newPoly.end() );
            
            //set all the points without reallocating control points
            for( std::size_t sz = 0; sz != newPoly.size(); ++sz )
            {
                pContour->set( sz, newPoly[ sz ].x, newPoly[ sz ].y );
            }
        }
    }
    virtual Site::Ptr GetInteractionSite() const
    {
        return boost::dynamic_pointer_cast< Site >( m_site.getPtr() );
    }

private:
    Site& m_site;
    Polygon2D m_originalPolygon;
    float m_startX, m_startY, m_qX, m_qY;
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
    virtual const Origin* getOrigin() const { return m_pSite.get(); }

    void matchFeatures();
    bool owns( IGlyph* pGlyph ) const;
    bool owns( const GlyphSpec* pGlyphSpec ) const;
    IGlyph::Ptr getMainGlyph() const { return m_pMainGlyph; }
    virtual void cmd_delete( const std::set< IGlyph* >& selection );
    
    virtual GlyphSpecProducer* fromGlyph( IGlyph* pGlyph ) const;
private:
    IEditContext& m_parent;
    Site::Ptr m_pSite;
    IGlyph::Ptr m_pMainGlyph;
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
inline IInteraction::Ptr make_interaction_ptr( Edit* pEdit, IInteraction* pInteraction )
{
    return IInteraction::Ptr( pInteraction, [ pEdit ]( IInteraction* p ){ pEdit->interaction_end( p ); } );
}

bool Edit::m_bViewArrangement = false;
bool Edit::m_bViewCellComplex = false;
bool Edit::m_bViewClearance = false;
    
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
    return make_interaction_ptr( this, m_pActiveInteraction );
}

IInteraction::Ptr Edit::interaction_draw( ToolMode toolMode, float x, float y, float qX, float qY, Site::Ptr pSite )
{
    ASSERT( !m_pActiveInteraction );
    
    if( toolMode == IEditContext::eContour )
    {
        x = Math::quantize_roundUp( x , qX );
        y = Math::quantize_roundUp( y , qY );
        
        if( Site* pSite = dynamic_cast< Site* >( m_pSite.get() ) )
        {
            IInteraction::Ptr pToolInteraction( new Polygon_Interaction( *pSite, x, y, qX, qY ) );
            interaction_evaluate();
            m_pActiveInteraction = new InteractionToolWrapper( *this, pToolInteraction );
            return make_interaction_ptr( this, m_pActiveInteraction );
        }
        else
        {
            //create new area
            Space::Ptr pNewSite( new Space( m_pSite, m_pSite->generateNewNodeName( "space" ) ) );
            pNewSite->init( x, y );
            m_pSite->add( pNewSite );
            interaction_evaluate();
            
            Edit* pNewEdit = dynamic_cast< Edit* >( getSiteContext( pNewSite ) );
            ASSERT( pNewEdit );

            IInteraction::Ptr pToolInteraction( new Polygon_Interaction( *pNewSite, 0, 0, qX, qY ) );
            pNewEdit->m_pActiveInteraction = new InteractionToolWrapper( *pNewEdit, pToolInteraction );
            pNewEdit->interaction_evaluate();
            return make_interaction_ptr( pNewEdit, pNewEdit->m_pActiveInteraction );
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
            //std::vector< wykobi::point2d< float > > points;
            for( Node::PtrVector::const_iterator 
                i = pSite->getChildren().begin(),
                iEnd = pSite->getChildren().end(); i!=iEnd; ++i )
            {
                if( Site::Ptr pClipSite = boost::dynamic_pointer_cast< Site >( *i ) )
                {
                    Site::Ptr pClipSiteCopy = 
                        boost::dynamic_pointer_cast< Site >( pClipSite->copy( m_pSite, 
                            m_pSite->generateNewNodeName( pClipSite ) ) );
                    m_pSite->add( pClipSiteCopy );
                    newSites.insert( pClipSiteCopy );
                } 
            }
            
            for( Site::PtrSet::iterator i = newSites.begin(),
                iEnd = newSites.end(); i!=iEnd; ++i )
            {
                Site::Ptr pClipSiteCopy = *i;
                
                Transform transform = pClipSiteCopy->getTransform();
                transform.translateBy( x, y );
                pClipSiteCopy->setTransform( transform );
            }
        }
        else if( pSite )
        {
            Site::Ptr pNewSite = boost::dynamic_pointer_cast< Site >( 
                pSite->copy( m_pSite, m_pSite->generateNewNodeName( pSite ) ) );
            pNewSite->init();
            
            Transform transform = pSite->getTransform();
            transform.translateBy( x, y );
            pNewSite->setTransform( transform );
            
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
        return make_interaction_ptr( this, m_pActiveInteraction );
    }
    else
    {
        THROW_RTE( "Unknown tool mode" );
    }
}

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
        }
    }
    return false;
}


void Edit::activated()
{
    interaction_evaluate();
}

void Edit::interaction_evaluate()
{
    //LOG_PROFILE_BEGIN( edit_interaction_evaluate );
    const Site::EvaluationMode mode = { m_bViewArrangement, m_bViewCellComplex, m_bViewClearance };

    Site::EvaluationResults results;
    m_pSite->evaluate( mode, results );
    
    interaction_update();

    //LOG_PROFILE_END( edit_interaction_evaluate );
}

void Edit::interaction_update()
{
    Site::PtrSet sites( m_pSite->getSites().begin(), m_pSite->getSites().end() );

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

    generics::for_each_second( m_glyphMap, []( boost::shared_ptr< SpaceGlyphs > pSpaceGlyphs ){ pSpaceGlyphs->interaction_update(); } );
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
        Clip::Ptr pClip( new Clip( Site::Ptr(), "clip" ) );
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
        Clip::Ptr pClip( new Clip( Site::Ptr(), "copy" ) );
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
    if( const Site* pSite = dynamic_cast< const Site* >( pGlyph->getGlyphSpec() ) )
    {
        Node::Ptr pNode = const_cast< Site* >( pSite )->getPtr();
        VERIFY_RTE( pNode );
        Site::Ptr pSitePtr = boost::dynamic_pointer_cast< Site >( pNode );
        VERIFY_RTE( pSitePtr );
        sites.push_back( pSitePtr );
    }
    return cmd_paste( sites, x, y, qX, qY );
}
IInteraction::Ptr Edit::cmd_paste( const std::set< IGlyph* >& selection, float x, float y, float qX, float qY )
{
    Site::PtrVector sites;
    for( std::set< IGlyph* >::iterator 
        i = selection.begin(),
        iEnd = selection.end(); i!=iEnd; ++i )
    {
        if( const Site* pSite = dynamic_cast< const Site* >( (*i)->getGlyphSpec() ) )
        {
            Node::Ptr pNode = const_cast< Site* >( pSite )->getPtr();
            VERIFY_RTE( pNode );
            Site::Ptr pSitePtr = boost::dynamic_pointer_cast< Site >( pNode );
            VERIFY_RTE( pSitePtr );
            sites.push_back( pSitePtr );
        }
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
        const std::string strNewKey = m_pSite->generateNewNodeName( *i );
        Site::Ptr pCopy = boost::dynamic_pointer_cast< Site >( 
            (*i)->copy( m_pSite, strNewKey ) );
        VERIFY_RTE( pCopy );
        m_pSite->add( pCopy );

        SpaceGlyphs::Ptr pSpaceGlyphs( new SpaceGlyphs( *this, pCopy, m_glyphFactory ) );
        m_glyphMap.insert( std::make_pair( pCopy, pSpaceGlyphs ) );

        copies.insert( pSpaceGlyphs->getMainGlyph() );
    }

    m_pActiveInteraction = new Interaction( *this, IEditContext::eArea, x, y, qX, qY, IGlyph::Ptr(), copies );
    pNewInteraction = make_interaction_ptr( this, m_pActiveInteraction );
    
    m_pSite->init();

    interaction_evaluate();

    return pNewInteraction;
}

void getSelectionBounds( const std::vector< Site* >& sites, Rect2D& transformBounds )
{
    Point2D ptBoundsTopLeft, ptBoundsBotRight;
    
    bool bFirst = true;
    for( const Site* pArea : sites )
    {
        if( boost::optional< Polygon2D > polyOpt = pArea->getContourPolygon() )
        {
            Polygon2D poly = polyOpt.get();
            for( Point2D& pt : poly )
                pArea->getTransform().transform( pt.x, pt.y );
        
            const Rect2D polyAABB = wykobi::aabb( poly );
            const Point2D ptTopLeft = wykobi::rectangle_corner( polyAABB, 0 );
            const Point2D ptBotRight  = wykobi::rectangle_corner( polyAABB, 2 );
            
            if( bFirst )
            {
                ptBoundsTopLeft     = ptTopLeft;
                ptBoundsBotRight    = ptBotRight;
                bFirst = false;
            }
            else
            {
                if( ptTopLeft.x < ptBoundsTopLeft.x )
                    ptBoundsTopLeft.x = ptTopLeft.x;
                if( ptTopLeft.y < ptBoundsTopLeft.y )
                    ptBoundsTopLeft.y = ptTopLeft.y;
                
                if( ptBotRight.x > ptBoundsBotRight.x )
                    ptBoundsBotRight.x = ptBotRight.x;
                if( ptBotRight.y > ptBoundsBotRight.y )
                    ptBoundsBotRight.y = ptBotRight.y;
            }
        }
    }
    
    transformBounds = 
        wykobi::make_rectangle< float >( 
            ptBoundsTopLeft, 
            ptBoundsBotRight );
}

void Edit::cmd_rotateLeft( const std::set< IGlyph* >& selection )
{
    std::vector< Site* > sites;
    for( std::set< IGlyph* >::iterator 
        i = selection.begin(),
        iEnd = selection.end(); i!=iEnd; ++i )
    {
        if( const Site* pSite = dynamic_cast< const Site* >( (*i)->getGlyphSpec() ) )
        {
            sites.push_back( const_cast< Site* >( pSite ) );
        }
    }
    
    Rect2D transformBounds;
    getSelectionBounds( sites, transformBounds );
    
    for( Site* pSite : sites )
    {
        pSite->cmd_rotateLeft( transformBounds );
    }

    interaction_evaluate();
}
void Edit::cmd_rotateRight( const std::set< IGlyph* >& selection )
{
    std::vector< Site* > areas;
    for( std::set< IGlyph* >::iterator 
        i = selection.begin(),
        iEnd = selection.end(); i!=iEnd; ++i )
    {
        if( const Site* pSite = dynamic_cast< const Site* >( (*i)->getGlyphSpec() ) )
            areas.push_back( const_cast< Site* >( pSite ) );
    }
    
    Rect2D transformBounds;
    getSelectionBounds( areas, transformBounds );
    
    for( Site* pArea : areas )
    {
        pArea->cmd_rotateRight( transformBounds );
    }

    interaction_evaluate();
}
void Edit::cmd_flipHorizontally( const std::set< IGlyph* >& selection )
{
    std::vector< Site* > areas;
    for( std::set< IGlyph* >::iterator 
        i = selection.begin(),
        iEnd = selection.end(); i!=iEnd; ++i )
    {
        if( const Site* pSite = dynamic_cast< const Site* >( (*i)->getGlyphSpec() ) )
            areas.push_back( const_cast< Site* >( pSite ) );
    }
    
    Rect2D transformBounds;
    getSelectionBounds( areas, transformBounds );
    
    for( Site* pArea : areas )
    {
        pArea->cmd_flipHorizontally( transformBounds );
    }

    interaction_evaluate();
}
void Edit::cmd_flipVertically( const std::set< IGlyph* >& selection )
{
    std::vector< Site* > areas;
    for( std::set< IGlyph* >::iterator 
        i = selection.begin(),
        iEnd = selection.end(); i!=iEnd; ++i )
    {
        if( const Site* pSite = dynamic_cast< const Site* >( (*i)->getGlyphSpec() ) )
            areas.push_back( const_cast< Site* >( pSite ) );
    }
    
    Rect2D transformBounds;
    getSelectionBounds( areas, transformBounds );
    
    for( Site* pArea : areas )
    {
        pArea->cmd_flipVertically( transformBounds );
    }

    m_pSite->init();
    interaction_evaluate();
}

void Edit::cmd_deleteProperties( const Node::PtrCstVector& nodes )
{
    std::set< Property::Ptr > uniqueProperties;
    for( Node::PtrCst pNodeCst : nodes )
    {
        if( Node::Ptr pNode = boost::const_pointer_cast< Node >( pNodeCst ) )
        {
            if( Property::Ptr pProperty = boost::dynamic_pointer_cast< Property >( pNode ) )
            {
                uniqueProperties.insert( pProperty );
            }
        }
    }
    
    for( Property::Ptr pProperty : uniqueProperties )
    {
        if( Node::Ptr pParent = pProperty->getParent() )
        {
            pParent->remove( pProperty );
        }
    }
    
    m_pSite->init();
    interaction_evaluate();
}

void Edit::cmd_addProperties( const Node::PtrCstVector& nodes, const std::string& strName, const std::string& strStatement )
{
    for( Node::PtrCst pNodeCst : nodes )
    {
        if( Node::Ptr pNode = boost::const_pointer_cast< Node >( pNodeCst ) )
        {
            Property::Ptr pProperty( new Property( pNode, strName ) );
            pProperty->setStatement( strStatement );
            pProperty->init();
            pNode->add( pProperty );
        }
    }
    
    m_pSite->init();
    interaction_evaluate();
}

void Edit::cmd_editProperties( const Node::PtrCstVector& nodes, const std::string& strName, const std::string& strStatement )
{
    for( Node::PtrCst pNodeCst : nodes )
    {
        if( Node::Ptr pNode = boost::const_pointer_cast< Node >( pNodeCst ) )
        {
            if( Property::Ptr pProperty = boost::dynamic_pointer_cast< Property >( pNode ) )
            {
                pProperty->setStatement( strStatement );
            }
        }
    }
    
    m_pSite->init();
    interaction_evaluate();
}
    
std::set< IGlyph* > Edit::generateExtrusion( float fAmount, bool bConvexHull )
{
    
    /*
    m_pSite->init();

    interaction_evaluate();*/
    
    std::set< IGlyph* > selection;
    /*{
        for( Site::Ptr pNewNode : newSites )
        {
            SiteMap::iterator iFind = m_glyphMap.find( pNewNode );
            if( iFind != m_glyphMap.end() )
                selection.insert( iFind->second->getMainGlyph().get() );
        }
    }*/
    
    return selection;
}

void Edit::save( const std::set< IGlyph* >& selection, const std::string& strFilePath )
{
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

    const boost::filesystem::path filePath = strFilePath;
    Blueprint::Ptr pBlueprint( new Blueprint( filePath.filename().replace_extension( "" ).string() ) );
    {
        pBlueprint->init();
        
        for( Site::PtrSet::iterator 
            i = sites.begin(),
            iEnd = sites.end(); i!=iEnd; ++i )
        {
            bool bResult = pBlueprint->add( (*i)->copy( pBlueprint, (*i)->Node::getName() ) );
            VERIFY_RTE( bResult );
        }
    }
    
    Factory factory;
    factory.save( pBlueprint, strFilePath );
}
    
void Edit::setViewMode( bool bArrangement, bool bCellComplex, bool bClearance )
{
    m_bViewArrangement  = bArrangement;
    m_bViewCellComplex  = bCellComplex;
    m_bViewClearance    = bClearance;
    
    interaction_evaluate();
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
        m_pMainGlyph = m_glyphFactory.createOrigin( m_pSite.get(), pParentGlyphs->getMainGlyph() );
    else
        m_pMainGlyph = m_glyphFactory.createOrigin( m_pSite.get(), IGlyph::Ptr() );

    MarkupText::List texts;
    m_pSite->getMarkupTexts( texts );
    for( MarkupText::List::const_iterator i = texts.begin(),
        iEnd = texts.end(); i!=iEnd; ++i )
    {
        if( IGlyph::Ptr pMarkupGlyph = m_glyphFactory.createMarkupText( *i, m_pMainGlyph ) )
            m_markup.insert( std::make_pair( *i, pMarkupGlyph ) );
    }

    MarkupPath::List paths;
    m_pSite->getMarkupPaths( paths );
    for( MarkupPath::List::const_iterator i = paths.begin(),
        iEnd = paths.end(); i!=iEnd; ++i )
    {
        if( IGlyph::Ptr pMarkupGlyph = m_glyphFactory.createMarkupPath( *i, m_pMainGlyph ) )
            m_markup.insert( std::make_pair( *i, pMarkupGlyph ) );
    }
    
    MarkupPolygonGroup::List polyGroups;
    m_pSite->getMarkupPolygonGroups( polyGroups );
    for( MarkupPolygonGroup::List::const_iterator i = polyGroups.begin(),
        iEnd = polyGroups.end(); i!=iEnd; ++i )
    {
        if( IGlyph::Ptr pMarkupGlyph = m_glyphFactory.createMarkupPolygonGroup( *i, m_pMainGlyph ) )
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

    Feature::PtrVector removals, additions, updates;
    generics::matchGetUpdates( m_features.begin(), m_features.end(), features.begin(), features.end(),
        generics::lessthan( generics::first< FeatureGlyphMap::const_iterator >(), 
                            generics::deref< Feature::PtrSet::const_iterator >() ),
                            []( std::map< Feature::Ptr, IGlyph::PtrSet >::const_iterator i, Feature::PtrSet::const_iterator j ){ return needUpdate( i, j ); },
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
                if( const Origin* pParentOrigin = dynamic_cast< const Origin* >( task.second->getParent() ) )
                {
                    ASSERT( m_pSite.get() == pParentOrigin );
                    pParentGlyph = m_pMainGlyph;
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
    
    m_pMainGlyph->update();
    generics::for_each_second( m_glyphs, []( IGlyph::Ptr pGlyph ){ pGlyph->update(); } );
    generics::for_each_second( m_markup, []( IGlyph::Ptr pGlyph ){ pGlyph->update(); } );
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
    return m_pMainGlyph.get() == pGlyph || 
        containsSecondPtr( m_features, pGlyph );
}

void SpaceGlyphs::cmd_delete( const std::set< IGlyph* >& selection )
{
    std::vector< const GlyphSpec* > selectionGlyphSpecs;
    for( IGlyph* pGlyph : selection )
    {
        selectionGlyphSpecs.push_back( pGlyph->getGlyphSpec());
    }
    
    std::set< GlyphSpecProducer* > glyphSpecProducers;
    for( std::set< IGlyph* >::iterator 
        i = selection.begin(),
        iEnd = selection.end(); i!=iEnd; ++i )
    {
        IGlyph* pGlyph = *i;
        if( GlyphSpecProducer* pGlyphPrd = const_cast< GlyphSpecProducer* >( fromGlyph( pGlyph ) ) )
        {
            if( !dynamic_cast< Site* >( pGlyphPrd ) )
            {
                glyphSpecProducers.insert( pGlyphPrd );
            }
        }
    }
    
    std::set< GlyphSpecProducer* > unhandled;
    for( GlyphSpecProducer* pGlyphSpecProducer : glyphSpecProducers )
    {
        if( !pGlyphSpecProducer->cmd_delete( selectionGlyphSpecs ) )
        {
            unhandled.insert( pGlyphSpecProducer );
        }
    }
    
    //if no handler delete the selection itself
    for( std::set< GlyphSpecProducer* >::iterator 
        i = unhandled.begin(),
        iEnd = unhandled.end(); i!=iEnd; ++i )
    {
        if( GlyphSpecProducer* pGlyphPrd = *i )
        {
            if( !dynamic_cast< Site* >( pGlyphPrd ) )
            {
                if( Node::Ptr pParent = pGlyphPrd->getParent() )
                {
                    pParent->remove( const_cast< GlyphSpecProducer* >( pGlyphPrd )->getPtr() );
                }
            }
        }
    }
    
    Edit::cmd_delete( selection );
}
}