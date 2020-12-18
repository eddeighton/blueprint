
#include "blueprint/editBase.h"

#include "blueprint/editNested.h"
#include "blueprint/editMain.h"
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

EditBase::EditBase( EditMain& editMain, GlyphFactory& glyphFactory, Site::Ptr pSite )
    :   m_editMain( editMain ),
        m_glyphFactory( glyphFactory ),
        m_pSite( pSite ),
        m_pActiveInteraction( 0u )
{
}

GlyphSpecProducer* EditBase::fromGlyph( IGlyph* pGlyph ) const
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


IInteraction::Ptr EditBase::interaction_start( ToolMode toolMode, Float x, Float y, Float qX, Float qY, IGlyph* pHitGlyph, const std::set< IGlyph* >& selection )
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

IInteraction::Ptr EditBase::interaction_draw( ToolMode toolMode, Float x, Float y, Float qX, Float qY, Site::Ptr pSite )
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
            
            EditBase* pNewEdit = dynamic_cast< EditBase* >( getSiteContext( pNewSite ) );
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
            //std::vector< wykobi::point2d< Float > > points;
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
                
                Matrix m = pClipSiteCopy->getTransform();
                {
                    Transform transform( x, y );
                    transform.transform( m );
                }
                pClipSiteCopy->setTransform( m );
            }
        }
        else if( pSite )
        {
            Site::Ptr pNewSite = boost::dynamic_pointer_cast< Site >( 
                pSite->copy( m_pSite, m_pSite->generateNewNodeName( pSite ) ) );
            pNewSite->init();
            
            Matrix m = pSite->getTransform();
            {
                Transform transform( x, y );
                transform.transform( m );
            }
            pNewSite->setTransform( m );
            
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

IEditContext* EditBase::getNestedContext( const std::vector< IGlyph* >& candidates )
{
    IEditContext* pEditContext = 0u;

    for( std::vector< IGlyph* >::const_iterator 
        i = candidates.begin(),
        iEnd = candidates.end(); i!=iEnd && !pEditContext; ++i )
    {
        for( EditNested::Map::const_iterator 
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

IEditContext* EditBase::getSiteContext( Site::Ptr pSite )
{
    SiteMap::const_iterator iFind = m_glyphMap.find( pSite );
    if( iFind != m_glyphMap.end() )
    {
        return iFind->second.get();
    }
    return nullptr;
}

bool EditBase::canEdit( IGlyph* pGlyph, ToolType toolType, ToolMode toolMode ) const
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


void EditBase::activated()
{
    interaction_evaluate();
}

void EditBase::interaction_evaluate()
{
    //LOG_PROFILE_BEGIN( edit_interaction_evaluate );
    
    const Site::EvaluationMode mode = m_editMain.getEvaluationMode();

    Site::EvaluationResults results;
    m_pSite->evaluate( mode, results );
    
    interaction_update();

    //LOG_PROFILE_END( edit_interaction_evaluate );
}

void EditBase::interaction_update()
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
    {
        m_glyphMap.erase( i->first );
    }

    for( Site::PtrVector::iterator 
        i = additions.begin(),
        iEnd = additions.end(); i!=iEnd; ++i )
    {
        m_glyphMap.insert( std::make_pair( *i, EditNested::Ptr( 
            new EditNested( m_editMain, *this, *i, m_glyphFactory ) ) ) );
    }

    generics::for_each_second( m_glyphMap, []( boost::shared_ptr< EditNested > pSpaceGlyphs ){ pSpaceGlyphs->interaction_update(); } );
}

void EditBase::interaction_end( IInteraction* pInteraction )
{
    ASSERT( m_pActiveInteraction == pInteraction );
    delete m_pActiveInteraction;
    m_pActiveInteraction = 0u;
}

void EditBase::cmd_delete( const std::set< IGlyph* >& selection )
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

Node::Ptr EditBase::cmd_cut( const std::set< IGlyph* >& selection )
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

Node::Ptr EditBase::cmd_copy( const std::set< IGlyph* >& selection )
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

IInteraction::Ptr EditBase::cmd_paste( Node::Ptr pPaste, Float x, Float y, Float qX, Float qY )
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

IInteraction::Ptr EditBase::cmd_paste( IGlyph* pGlyph, Float x, Float y, Float qX, Float qY )
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
IInteraction::Ptr EditBase::cmd_paste( const std::set< IGlyph* >& selection, Float x, Float y, Float qX, Float qY )
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

IInteraction::Ptr EditBase::cmd_paste( Site::PtrVector sites, Float x, Float y, Float qX, Float qY )
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

        EditNested::Ptr pSpaceGlyphs( new EditNested( m_editMain, *this, pCopy, m_glyphFactory ) );
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
    THROW_RTE( "TODO" );
    /*
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
        wykobi::make_rectangle< Float >( 
            ptBoundsTopLeft, 
            ptBoundsBotRight );*/
}

void EditBase::cmd_rotateLeft( const std::set< IGlyph* >& selection )
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
void EditBase::cmd_rotateRight( const std::set< IGlyph* >& selection )
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
void EditBase::cmd_flipHorizontally( const std::set< IGlyph* >& selection )
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
void EditBase::cmd_flipVertically( const std::set< IGlyph* >& selection )
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

void EditBase::cmd_deleteProperties( const Node::PtrCstVector& nodes )
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

void EditBase::cmd_addProperties( const Node::PtrCstVector& nodes, const std::string& strName, const std::string& strStatement )
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

void EditBase::cmd_editProperties( const Node::PtrCstVector& nodes, const std::string& strName, const std::string& strStatement )
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
    
std::set< IGlyph* > EditBase::generateExtrusion( Float fAmount, bool bConvexHull )
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

void EditBase::save( const std::set< IGlyph* >& selection, const std::string& strFilePath )
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



}