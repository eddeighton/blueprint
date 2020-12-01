
#include "blueprint/edit.h"

#include "blueprint/blueprint.h"
#include "blueprint/spaceUtils.h"

#include "ed/ed.hpp"

#include "common/compose.hpp"
#include "common/assert_verify.hpp"

#include "wykobi.hpp"
#include "wykobi_algorithm.hpp"

#include <boost/numeric/conversion/bounds.hpp>

#include <algorithm>
#include <iomanip>

namespace Blueprint
{

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
const std::string& Blueprint::TypeName()
{
    static const std::string strTypeName( "blueprint" );
    return strTypeName;
}
Blueprint::Blueprint( const std::string& strName )
    :   Site( Ptr(), strName )
{

}

Blueprint::Blueprint( PtrCst pOriginal, Node::Ptr pNotUsed, const std::string& strName )
    :   Site( pOriginal, Ptr(), strName )
{
    ASSERT( !pNotUsed );
}

Node::Ptr Blueprint::copy( Node::Ptr pParent, const std::string& strName ) const
{   
    return Node::copy< Blueprint >( shared_from_this(), pParent, strName );
}

void Blueprint::init()
{
    Site::init();
}


bool Blueprint::add( Node::Ptr pNewNode )
{
    return Site::add( pNewNode );
}

void Blueprint::remove( Node::Ptr pNode )
{
    Site::remove( pNode );
}

void Blueprint::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );
}

void Blueprint::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Site::save( node );
}

void Blueprint::evaluate( const EvaluationMode& mode, EvaluationResults& results )
{
    for( Site::PtrVector::iterator i = m_sites.begin(),
        iEnd = m_sites.end(); i!=iEnd; ++i )
        (*i)->evaluate( mode, results );
}




}