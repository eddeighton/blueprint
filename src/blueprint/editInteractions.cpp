
#include "blueprint/editInteractions.h"
#include "blueprint/editBase.h"
#include "blueprint/cgalUtils.h"

namespace Blueprint
{
    
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Interaction::Interaction( EditBase& edit, IEditContext::ToolMode toolMode, 
                Float x, Float y, Float qX, Float qY, 
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
                    
                    const Vector v = getTranslation( p->getOrigin()->getTransform() );
                    
                    m_initialValues.push_back( 
                        InitialValue( CGAL::to_double( v.x() ), 
                                      CGAL::to_double( v.y() ) ) );
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
                    m_initialValues.push_back( 
                        InitialValue( 
                            CGAL::to_double( p->getControlPoint()->getX() ), 
                            CGAL::to_double( p->getControlPoint()->getY() ) ) );
                }
            }
        }
    }
}

void Interaction::OnMove( Float x, Float y )
{
    Float fDeltaX = Math::quantize_roundUp( x - m_startX, m_qX );
    Float fDeltaY = Math::quantize_roundUp( y - m_startY, m_qY );

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

Site::Ptr Interaction::GetInteractionSite() const
{
    return m_edit.getSite();
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
InteractionToolWrapper::InteractionToolWrapper( EditBase& edit, IInteraction::Ptr pWrapped )
    :   m_edit( edit ),
        m_pToolInteraction( pWrapped )
{
}

void InteractionToolWrapper::OnMove( Float x, Float y )
{
    m_pToolInteraction->OnMove( x, y );
    m_edit.interaction_evaluate();
}

Site::Ptr InteractionToolWrapper::GetInteractionSite() const
{
    return m_edit.getSite();
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Polygon_Interaction::Polygon_Interaction( Site& site, Float x, Float y, Float qX, Float qY )
    :   m_site( site ),
        m_qX( qX ),
        m_qY( qY )
{
    m_startX = x = Math::quantize_roundUp( x, qX );
    m_startY = y = Math::quantize_roundUp( y, qY );

    if( Feature_Contour::Ptr pContour = m_site.getContour() )
    {
        m_originalPolygon = pContour->getPolygon();
        
        const Point pt( m_startX, m_startY );
        Polygon newPoly = m_originalPolygon;
        newPoly.push_back( pt );
        pContour->set( newPoly );
        
        OnMove( x, y );
    }
}

void Polygon_Interaction::OnMove( Float x, Float y )
{
    if( Feature_Contour::Ptr pContour = m_site.getContour() )
    {
        const Float fDeltaX = Math::quantize_roundUp( x, m_qX );
        const Float fDeltaY = Math::quantize_roundUp( y, m_qY );
        const Point pt( fDeltaX, fDeltaY );
        
        unsigned int uiIndex = Utils::getClosestPoint( m_originalPolygon, pt );
        
        VERIFY_RTE( uiIndex < m_originalPolygon.size() || uiIndex == 0 );
        //if( m_originalPolygon.size() > 0 )
        //    uiIndex = ( uiIndex + 1 ) % m_originalPolygon.size();
        
        Polygon newPoly;
        for( std::size_t sz = 0U; ( sz != uiIndex ) && ( sz < m_originalPolygon.size() ); ++sz )
            newPoly.push_back( m_originalPolygon[ sz ] );
        newPoly.push_back( pt );
        for( std::size_t sz = uiIndex; sz < m_originalPolygon.size(); ++sz )
            newPoly.push_back( m_originalPolygon[ sz ] );
        
        if( !newPoly.is_empty() && newPoly.is_simple() && !newPoly.is_counterclockwise_oriented() )
        {
            std::reverse( newPoly.begin(), newPoly.end() );
        }
        
        //set all the points without reallocating control points
        for( std::size_t sz = 0; sz != newPoly.size(); ++sz )
        {
            pContour->set( sz, 
                CGAL::to_double( newPoly[ sz ].x() ), 
                CGAL::to_double( newPoly[ sz ].y() ) );
        }
    }
}
    
Site::Ptr Polygon_Interaction::GetInteractionSite() const
{
    return boost::dynamic_pointer_cast< Site >( m_site.getPtr() );
}

}