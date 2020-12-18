
#include "blueprint/spacePolyInfo.h"

namespace Blueprint
{
/*
inline void loaddata( std::istream& is, Point2D& pt )
{
    is >> pt.x >> pt.y;
}
inline void loaddata( std::istream& is, Wall& data )
{
    is >> data.m_bClosed;
    is >> data.m_bCounterClockwise;
    std::size_t szSize;
    is >> szSize;
    for( std::size_t sz = 0; sz != szSize; ++sz )
    {
        Point2D pt;
        loaddata( is, pt );
        data.points.push_back( pt );
    }
}
inline void loaddata( std::istream& is, Polygon2D& data )
{
    std::size_t szSize;
    is >> szSize;
    for( std::size_t sz = 0; sz != szSize; ++sz )
    {
        Point2D pt;
        loaddata( is, pt );
        data.push_back( pt );
    }
}
inline void loaddata( std::istream& is, PolygonWithHoles& data )
{
    loaddata( is, data.outer );
    std::size_t szSize;
    is >> szSize;
    for( std::size_t sz = 0; sz != szSize; ++sz )
    {
        Polygon2D hole;
        loaddata( is, hole );
        data.holes.emplace_back( hole );
    }
}
          
void SpacePolyInfo::load( std::istream& is )
{
    {
        std::size_t szSize;
        is >> szSize;
        for( std::size_t sz = 0U; sz != szSize; ++sz )
        {
            PolygonWithHoles p;
            loaddata( is, p );
            floors.emplace_back( p );
        }
    }
    {
        std::size_t szSize;
        is >> szSize;
        for( std::size_t sz = 0U; sz != szSize; ++sz )
        {
            Polygon2D p;
            loaddata( is, p );
            fillers.emplace_back( p );
        }
    }
    {
        std::size_t szSize;
        is >> szSize;
        for( std::size_t sz = 0U; sz != szSize; ++sz )
        {
            Wall p( false, false );
            loaddata( is, p );
            walls.emplace_back( p );
        }
    }
}

inline void savedata( std::ostream& os, const Point2D& pt )
{
    os << pt.x << ' ' << pt.y << '\n';
}
         
inline void savedata( std::ostream& os, const Wall& data )
{
    os << data.m_bClosed << '\n';
    os << data.m_bCounterClockwise << '\n';
    os << data.points.size() << '\n';
    for( const Point2D& pt : data.points )
        savedata( os, pt );
}       
       
inline void savedata( std::ostream& os, const Polygon2D& data )
{
    os << data.size() << '\n';
    for( const Point2D& pt : data )
        savedata( os, pt );
}
            
inline void savedata( std::ostream& os, const PolygonWithHoles& data )
{
    savedata( os, data.outer );
    os << data.holes.size() << '\n';
    for( const Polygon2D& p : data.holes )
        savedata( os, p );
}

 
void SpacePolyInfo::save( std::ostream& os )
{
    os << floors.size() << '\n';
    for( const PolygonWithHoles& data : floors )
        savedata( os, data );
    os << fillers.size() << '\n';
    for( const Polygon2D& data : fillers )
        savedata( os, data );
    os << walls.size() << '\n';
    for( const Wall& data : walls )
        savedata( os, data );
}
*/

}