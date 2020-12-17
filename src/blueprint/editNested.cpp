
#include "blueprint/editNested.h"

namespace Blueprint
{

GlyphSpecProducer* EditNested::fromGlyph( IGlyph* pGlyph ) const
{
    GlyphSpecProducer* pResult = EditBase::fromGlyph( pGlyph );
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

EditNested::EditNested( EditMain& editMain, IEditContext& parentContext, Site::Ptr pSpace, GlyphFactory& glyphFactory )
    :   EditBase( editMain, glyphFactory, pSpace ),
        m_parent( parentContext )
{
    //create the main image glyph
    if( EditNested* pParentGlyphs = dynamic_cast< EditNested* >( &m_parent ) )
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

void EditNested::matchFeatures()
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

void EditNested::interaction_update()
{
    //recurse
    EditBase::interaction_update();

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

bool EditNested::owns( const GlyphSpec* pGlyphSpec ) const
{
    return m_glyphs.find( pGlyphSpec ) != m_glyphs.end();
}

bool EditNested::owns( IGlyph* pGlyph ) const
{
    return m_pMainGlyph.get() == pGlyph || 
        containsSecondPtr( m_features, pGlyph );
}

void EditNested::cmd_delete( const std::set< IGlyph* >& selection )
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
    
    EditBase::cmd_delete( selection );
}

}
