
#ifndef VISIBILITY_15_DEC_2020
#define VISIBILITY_15_DEC_2020

#include "blueprint/cgalSettings.h"
#include "blueprint/compilation.h"

#include <memory>

namespace Blueprint
{
    
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class FloorAnalysis
{
    friend class Analysis;
    FloorAnalysis();
public:
    using VertexHandle = 
        Arrangement::Vertex_const_handle;
        
    FloorAnalysis( Compilation& compilation, boost::shared_ptr< Blueprint > pBlueprint );
    
    const Arrangement& getFloor() const { return m_arr; }
    const Arrangement::Face_const_handle getFloorFace() const { return m_hFloorFace; }
    
    bool isWithinFloor( VertexHandle v1, VertexHandle v2 ) const;
    boost::optional< Curve > getFloorBisector( VertexHandle v1, VertexHandle v2, bool bKeepSingleEnded ) const;
    boost::optional< Curve > getFloorBisector( const Segment& segment, bool bKeepSingleEnded ) const;
    
    void render( const boost::filesystem::path& filepath );
    
    void save( std::ostream& os ) const;
    void load( std::istream& is );
private:
    void findFloorFace();
    void calculateBounds();

    void recurseObjects( Site::Ptr pSpace );

    mutable Arrangement m_arr;
    Arrangement::Face_handle m_hFloorFace;
    Rect m_boundingBox;
    
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class Visibility
{
    friend class Analysis;
    
    Visibility();
public:
    Visibility( FloorAnalysis& floor );
    
    const Arrangement& getArrangement() const { return m_arr; }
    
    void render( const boost::filesystem::path& filepath );
    
    void save( std::ostream& os ) const;
    void load( std::istream& is );
    
private:
    Arrangement m_arr;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class Analysis
{
    Analysis();
    Analysis( boost::shared_ptr< Blueprint > pBlueprint );
public:
    using Ptr = std::shared_ptr< Analysis >;

    static Ptr constructFromBlueprint( boost::shared_ptr< Blueprint > pBlueprint );
    static Ptr constructFromStream( std::istream& is );
    
    struct IPainter
    {
        virtual void moveTo( float x, float y ) = 0;
        virtual void lineTo( float x, float y ) = 0;
    };
    
    void renderFloor( IPainter& painter ) const;

    void save( std::ostream& os ) const;
    
private:
    Compilation     m_compilation;
    FloorAnalysis   m_floor;
    Visibility      m_visibility;
};

}

#endif //VISIBILITY_15_DEC_2020