
#ifndef VISIBILITY_15_DEC_2020
#define VISIBILITY_15_DEC_2020

#include "blueprint/cgalSettings.h"
#include "blueprint/compilation.h"

#include <memory>

namespace Blueprint
{
    
class FloorAnalysis
{
public:
    using VertexHandle = 
        Arr_with_hist_2::Vertex_const_handle;
        
    FloorAnalysis( Compilation& compilation );
    
    const Arr_with_hist_2& getFloor() const { return m_arr; }
    const Arr_with_hist_2::Face_const_handle getFloorFace() const { return m_hFloorFace; }
    
    bool isWithinFloor( VertexHandle v1, VertexHandle v2 ) const;
    boost::optional< Curve_2 > getFloorBisector( VertexHandle v1, VertexHandle v2, bool bKeepSingleEnded ) const;
    boost::optional< Curve_2 > getFloorBisector( const Segment_2& segment, bool bKeepSingleEnded ) const;
    
    void render( const boost::filesystem::path& filepath );
private:
    void recurseObjects( Site::Ptr pSpace );

    mutable Arr_with_hist_2 m_arr;
    Arr_with_hist_2::Face_handle m_hFloorFace;
    Rect_2 m_boundingBox;
    
};

class Visibility
{
public:
    Visibility( FloorAnalysis& floor );
    
    void render( const boost::filesystem::path& filepath );
private:
    FloorAnalysis& m_floor;
    Arr_with_hist_2 m_arr;
};

}

#endif //VISIBILITY_15_DEC_2020