#ifndef EDIT_14_09_2013
#define EDIT_14_09_2013

#include "blueprint/editBase.h"

namespace Blueprint
{

class EditMain : public EditBase
{
    EditMain( GlyphFactory& glyphFactory, Site::Ptr pSite,
        bool bArrangement, bool bCellComplex, bool bClearance,
        const std::string& strFilePath );
public:
    using Ptr = boost::shared_ptr< EditMain >;
    
    static EditMain::Ptr create( 
        GlyphFactory& glyphFactory, 
        Site::Ptr pSite, 
        bool bArrangement, bool bCellComplex, bool bClearance,
        const std::string& strFilePath = std::string() );
    
    const std::string getFilePath() const { return m_strFilePath; }
    void setFilePath( const std::string& strFilePath ) { m_strFilePath = strFilePath; }
    void setViewMode( bool bArrangement, bool bCellComplex, bool bClearance );
    
    const Site::EvaluationMode getEvaluationMode() const;
    
private:
    std::string m_strFilePath;
    bool m_bViewArrangement, m_bViewCellComplex, m_bViewClearance;
};

}


#endif //EDIT_14_09_2013