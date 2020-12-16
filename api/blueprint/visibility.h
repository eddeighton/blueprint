
#ifndef VISIBILITY_15_DEC_2020
#define VISIBILITY_15_DEC_2020

#include "blueprint/cgalSettings.h"
#include "blueprint/compilation.h"

namespace Blueprint
{
    
class FloorAnalysis
{
public:
    FloorAnalysis( Compilation& compilation );
    
    const Arr_with_hist_2::Face_const_handle getFloorFace() const { return m_hFloorFace; }
    
    void render( const boost::filesystem::path& filepath );
private:
    void recurseObjects( Site::Ptr pSpace );

    Arr_with_hist_2 m_arr;
    Arr_with_hist_2::Face_handle m_hFloorFace;
};

class Visibility
{
public:
    Visibility( FloorAnalysis& floor );
    
    void render( const boost::filesystem::path& filepath );
private:
    Arr_with_hist_2 m_arr;
    Iso_rectangle_2 m_boundingBox;
};

}

#endif //VISIBILITY_15_DEC_2020