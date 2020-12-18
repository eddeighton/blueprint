#include "blueprint/site.h"

#include "blueprint/markup.h"
#include "blueprint/rasteriser.h"

#include "common/compose.hpp"
#include "common/assert_verify.hpp"

#include <algorithm>

namespace Blueprint
{
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Site::Site( Site::Ptr pParent, const std::string& strName )
    :   GlyphSpecProducer( pParent, strName ),
        m_pSiteParent( pParent )
{

}

Site::Site( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   GlyphSpecProducer( pOriginal, pParent, strName ),
        m_pSiteParent( pParent )
{
    m_transform = pOriginal->m_transform;
}

std::string Site::getStatement() const
{
    std::ostringstream os;
    {
        Ed::Shorthand sh;
        {
            Ed::OShorthandStream ossh( sh );
            ossh << m_transform;
        }
        os << sh;
    }
    return os.str();
}

void Site::load( Factory& factory, const Ed::Node& node )
{
    Node::load( getPtr(), factory, node );
    
    if( boost::optional< const Ed::Shorthand& > shOpt = node.getShorty() )
    {
        Ed::IShorthandStream is( shOpt.get() );
        is >> m_transform;
    }
}

void Site::save( Ed::Node& node ) const
{
    Node::save( node );
    
    if( !node.statement.shorthand ) 
    {
        node.statement.shorthand = Ed::Shorthand();
    }
    
    {
        Ed::OShorthandStream os( node.statement.shorthand.get() );
        os << m_transform;
    }
}

void Site::init()
{
    m_sites.clear();
    for_each( generics::collectIfConvert( m_sites, Node::ConvertPtrType< Site >(), Node::ConvertPtrType< Site >() ) );

    if( !m_pContourPathImpl.get() )
        m_pContourPathImpl.reset( new SimplePolygonMarkup( this, m_contourPolygon, true ) );
    
    m_strLabelText.clear();
    m_properties.clear();
    {
        std::ostringstream os;
        os << Node::getName();
        {
            for_each_recursive( 
                generics::collectIfConvert( m_properties, 
                    Node::ConvertPtrType< Property >(), 
                    Node::ConvertPtrType< Property >() ),
                    Node::ConvertPtrType< Site >() );
                    
            for( Property::Ptr pProperty : m_properties )
            {
                os << "\n" << pProperty->getName() << ": " << pProperty->getValue();
            }
        }
        m_strLabelText = os.str();
    }

    if( !m_pLabel.get() )
        m_pLabel.reset( new TextImpl( this, m_strLabelText, 0.0f, 0.0f ) ); 
    
    GlyphSpecProducer::init();
}

void Site::evaluate( const EvaluationMode& mode, EvaluationResults& results )
{
    //bottom up recursion
    for( PtrVector::iterator i = m_sites.begin(),
        iEnd = m_sites.end(); i!=iEnd; ++i )
    {
        (*i)->evaluate( mode, results );
    }
}
    
bool Site::add( Node::Ptr pNewNode )
{
    const bool bAdded = Node::add( pNewNode );
    if( bAdded )
    {
        if( Site::Ptr pNewSite = boost::dynamic_pointer_cast< Site >( pNewNode ) )
            m_sites.push_back( pNewSite );
    }
    return bAdded;
}

void Site::remove( Node::Ptr pNode )
{
    Node::remove( pNode );
    if( Site::Ptr pOldSite = boost::dynamic_pointer_cast< Site >( pNode ) )
    {
        Site::PtrVector::iterator iFind = std::find( m_sites.begin(), m_sites.end(), pOldSite );
        VERIFY_RTE( iFind != m_sites.end() );
        if( iFind != m_sites.end() )
            m_sites.erase( iFind );
    }
}

Transform Site::getAbsoluteTransform() const
{
    Transform transform;
    Site::PtrCst pIter = boost::dynamic_pointer_cast< const Site >( getPtr() );
    while( pIter )
    {
        transform = pIter->getTransform() * transform;
        pIter = boost::dynamic_pointer_cast< const Site >( pIter->Node::getParent() );
    }
    return transform;
}


void Site::setTransform( const Transform& transform )
{ 
    m_transform = transform; 
    setModified();
}

void Site::cmd_rotateLeft( const Rect& transformBounds )
{
    m_transform = rotateLeft( m_transform, transformBounds );
    setModified();
}

void Site::cmd_rotateRight( const Rect& transformBounds )
{
    m_transform = rotateRight( m_transform, transformBounds );
    setModified();
}

void Site::cmd_flipHorizontally( const Rect& transformBounds )
{
    m_transform = flipHorizontally( m_transform, transformBounds );
    setModified();
}

void Site::cmd_flipVertically( const Rect& transformBounds )
{
    m_transform = flipVertically( m_transform, transformBounds );
    setModified();
}

}