
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
        Arrangement::Vertex_const_handle;
        
    FloorAnalysis( Compilation& compilation );
    
    const Arrangement& getFloor() const { return m_arr; }
    const Arrangement::Face_const_handle getFloorFace() const { return m_hFloorFace; }
    
    bool isWithinFloor( VertexHandle v1, VertexHandle v2 ) const;
    boost::optional< Curve > getFloorBisector( VertexHandle v1, VertexHandle v2, bool bKeepSingleEnded ) const;
    boost::optional< Curve > getFloorBisector( const Segment& segment, bool bKeepSingleEnded ) const;
    
    void render( const boost::filesystem::path& filepath );
private:
    void recurseObjects( Site::Ptr pSpace );

    mutable Arrangement m_arr;
    Arrangement::Face_handle m_hFloorFace;
    Rect m_boundingBox;
    
};

class Visibility
{
public:
    Visibility( FloorAnalysis& floor );
    
    void render( const boost::filesystem::path& filepath );
private:
    FloorAnalysis& m_floor;
    Arrangement m_arr;
};

}

#endif //VISIBILITY_15_DEC_2020