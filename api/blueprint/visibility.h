
#ifndef VISIBILITY_15_DEC_2020
#define VISIBILITY_15_DEC_2020

#include "blueprint/cgalSettings.h"
#include "blueprint/compilation.h"

namespace Blueprint
{
    
class Visibility
{
public:
    Visibility( Compilation& compilation );
    
    void render( const boost::filesystem::path& filepath );
private:
    void recurseObjects( Site::Ptr pSpace );


    Arr_with_hist_2 m_arr;
};

}

#endif //VISIBILITY_15_DEC_2020