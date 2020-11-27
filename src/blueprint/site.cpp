#include "blueprint/site.h"

#include "common/compose.hpp"
#include "common/assert_verify.hpp"

#include <algorithm>

namespace Blueprint
{
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Site::Site( Node::Ptr pParent, const std::string& strName )
    :   GlyphSpecProducer( pParent, strName )
{

}

Site::Site( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName )
    :   GlyphSpecProducer( pOriginal, pParent, strName )
{
}

void Site::init()
{
    m_spaces.clear();
    for_each( generics::collectIfConvert( m_spaces, Node::ConvertPtrType< Site >(), Node::ConvertPtrType< Site >() ) );

    GlyphSpecProducer::init();
}

bool Site::add( Node::Ptr pNewNode )
{
    const bool bAdded = Node::add( pNewNode );
    if( bAdded )
    {
        if( Site::Ptr pNewSite = boost::dynamic_pointer_cast< Site >( pNewNode ) )
            m_spaces.push_back( pNewSite );
    }
    return bAdded;
}

void Site::remove( Node::Ptr pNode )
{
    Node::remove( pNode );
    if( Site::Ptr pOldSite = boost::dynamic_pointer_cast< Site >( pNode ) )
    {
        Site::PtrVector::iterator iFind = std::find( m_spaces.begin(), m_spaces.end(), pOldSite );
        VERIFY_RTE( iFind != m_spaces.end() );
        if( iFind != m_spaces.end() )
            m_spaces.erase( iFind );
    }
}

Matrix Site::getAbsoluteTransform() const
{
    Matrix transform;
    Site::PtrCst pIter = boost::dynamic_pointer_cast< const Site >( getPtr() );
    while( pIter )
    {
        pIter->getTransform().transform( transform );
        pIter = boost::dynamic_pointer_cast< const Site >( pIter->Node::getParent() );
    }
    return transform;
}

}