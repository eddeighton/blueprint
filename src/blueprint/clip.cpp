#include "blueprint/clip.h"

#include "blueprint/edit.h"

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

Clip::Clip( Node::Ptr pParent, const std::string& strName )
:   Site( pParent, strName )
{
}

Clip::Clip( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName )
:   Site( pOriginal, pParent, strName )
{
}

}
