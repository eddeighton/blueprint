#include "blueprint/clip.h"

#include "ed/ed.hpp"

#include "common/rounding.hpp"

#include <cmath>

namespace Blueprint
{
    
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
const std::string& Clip::TypeName()
{
    static const std::string strTypeName( "clip" );
    return strTypeName;
}

Clip::Clip( Site::Ptr pParent, const std::string& strName )
:   Site( pParent, strName )
{
}

Clip::Clip( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
:   Site( pOriginal, pParent, strName )
{
}

Node::Ptr Clip::copy( Node::Ptr pParent, const std::string& strName ) const 
{ 
    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
    VERIFY_RTE( pSiteParent || !pParent );
    return Node::copy< Clip >( 
        boost::dynamic_pointer_cast< const Clip >( shared_from_this() ), pSiteParent, strName );
}

}
