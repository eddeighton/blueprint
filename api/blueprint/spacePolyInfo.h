
#ifndef SPACE_POLY_INFO_15_DEC_2020
#define SPACE_POLY_INFO_15_DEC_2020

#include "blueprint/geometry.h"

#include <memory>
#include <vector>
#include <istream>
#include <ostream>

namespace Blueprint
{

struct PolygonWithHoles
{
    Polygon2D outer;
    std::vector< Polygon2D > holes;
};

struct Wall
{
    friend class Compilation;
public:
    Wall( bool bClosed, bool bCounterClockwise )
        :   m_bClosed( bClosed ), 
            m_bCounterClockwise( bCounterClockwise )
    {}
    
    bool isClosed() const { return m_bClosed; }
    bool isCounterClockwise() const { return m_bCounterClockwise; }
    
    bool m_bClosed, m_bCounterClockwise;
    Polygon2D points;
};

struct SpacePolyInfo
{
    using Ptr = std::shared_ptr< SpacePolyInfo >;
    
    void load( std::istream& is );
    void save( std::ostream& os );
    
    std::vector< PolygonWithHoles > floors;
    std::vector< Polygon2D > fillers;
    std::vector< Wall > walls;
};

}

#endif //SPACE_POLY_INFO_15_DEC_2020