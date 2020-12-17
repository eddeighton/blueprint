#include "blueprint/editMain.h"

#include "blueprint/editNested.h"
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
    
EditMain::EditMain( GlyphFactory& glyphFactory, 
                    Site::Ptr pSite,
                    bool bArrangement, 
                    bool bCellComplex, 
                    bool bClearance, 
                    const std::string& strFilePath )
    :   EditBase( *this, glyphFactory, pSite ),
        m_strFilePath( strFilePath ),
        m_bViewArrangement( bArrangement ),
        m_bViewCellComplex( bCellComplex ),
        m_bViewClearance  ( bClearance )
{
}

EditMain::Ptr EditMain::create( GlyphFactory& glyphFactory, 
                                Site::Ptr pSite, 
                                bool bArrangement, 
                                bool bCellComplex, 
                                bool bClearance,
                                const std::string& strFilePath )
{
    EditMain::Ptr pNewEdit;

    Blueprint::Ptr pBlueprint;

    if( Clip::Ptr pClip = boost::dynamic_pointer_cast< Clip >( pSite ) )
    {
        pBlueprint.reset( new Blueprint( pClip->Node::getName() ) );
        for( Node::PtrVector::const_iterator 
            i = pClip->Node::getChildren().begin(),
            iEnd = pClip->Node::getChildren().end(); i!=iEnd; ++i )
            pBlueprint->add( (*i)->copy( pBlueprint, (*i)->Node::getName() ) );

        pNewEdit.reset( new EditMain( glyphFactory, pBlueprint, 
                                bArrangement, bCellComplex, bClearance, strFilePath ) );
        pNewEdit->interaction_evaluate();
    }
    else if( pBlueprint = boost::dynamic_pointer_cast< Blueprint >( pSite ) )
    {
        pNewEdit.reset( new EditMain( glyphFactory, pSite, 
                                bArrangement, bCellComplex, bClearance, strFilePath ) );
        pNewEdit->interaction_evaluate();
    }
    else
    {
        pBlueprint.reset( new Blueprint( pSite->Node::getName() ) );
        pBlueprint->add( pSite->copy( pBlueprint, pSite->Node::getName() ) );
    }
    
    VERIFY_RTE( pBlueprint );

    pNewEdit.reset( new EditMain( glyphFactory, pBlueprint, 
                                bArrangement, bCellComplex, bClearance, strFilePath ) );
    pNewEdit->interaction_evaluate();

    return pNewEdit;
}
  
const Site::EvaluationMode EditMain::getEvaluationMode() const
{
    const Site::EvaluationMode mode = { m_bViewArrangement, m_bViewCellComplex, m_bViewClearance };
    return mode;
}

void EditMain::setViewMode( bool bArrangement, bool bCellComplex, bool bClearance )
{
    m_bViewArrangement  = bArrangement;
    m_bViewCellComplex  = bCellComplex;
    m_bViewClearance    = bClearance;
    
    interaction_evaluate();
}

}