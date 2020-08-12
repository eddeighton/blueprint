#ifndef SERIALISATION
#define SERIALISATION

#include "wykobi.hpp"

#include <ostream>
#include <istream>
#include <iterator>
#include <iomanip>

namespace Ed
{
    inline OShorthandStream& operator<<( OShorthandStream& os, const wykobi::point2d< float >& pt )
    {
        return os << pt.x << pt.y;
    }
    inline IShorthandStream& operator>>( IShorthandStream& is, wykobi::point2d< float >& pt )
    {
        return is >> pt.x >> pt.y;
    }
    
    inline OShorthandStream& operator<<( OShorthandStream& os, const wykobi::polygon< float, 2 >& polygon )
    {
        os << polygon.size();
        for( auto i = polygon.begin(), iEnd = polygon.end(); i!=iEnd; ++i )
            os << *i;
        return os;
    }
    inline IShorthandStream& operator>>( IShorthandStream& is, wykobi::polygon< float, 2 >& polygon )
    {
        std::size_t size = 0;
        is >> size;
        for( size_t i = 0; i != size; ++i )
        {
            wykobi::point2d< float > pt;
            is >> pt;
            polygon.push_back( pt );
        }
        return is;
    }
    inline std::ostream& operator<<( std::ostream& os, const wykobi::point2d< float >& pt )
    {
        return os << pt.x << pt.y;
    }
    inline std::istream& operator>>( std::istream& is, wykobi::point2d< float >& pt )
    {
        return is >> pt.x >> pt.y;
    }
    
    inline std::ostream& operator<<( std::ostream& os, const wykobi::polygon< float, 2 >& polygon )
    {
        os << polygon.size();
        for( auto i = polygon.begin(), iEnd = polygon.end(); i!=iEnd; ++i )
            os << *i;
        return os;
    }
    inline std::istream& operator>>( std::istream& is, wykobi::polygon< float, 2 >& polygon )
    {
        std::size_t size = 0;
        is >> size;
        for( size_t i = 0; i != size; ++i )
        {
            wykobi::point2d< float > pt;
            is >> pt;
            polygon.push_back( pt );
        }
        return is;
    }
}

#endif// SERIALISATION