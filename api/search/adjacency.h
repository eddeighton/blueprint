#ifndef ADJACENCY_15_09_2013
#define ADJACENCY_15_09_2013

#include "common/math.hpp"
#include "common/angle.hpp"

namespace Search
{

struct AdjacencyTraits_8WayDiscrete
{
    typedef char Iterator;
    template< class T >
    static Iterator Begin( T )    { return EdsMath::Angle< 8 >::eEast; }
    template< class T >
    static Iterator End( T )      { return EdsMath::Angle< 8 >::TOTAL_ANGLES; }
    static void Increment( Iterator& iter )
    {
        ++iter;
    }

    template< class TVectorType >
    static inline void adjacent( const TVectorType& v, Iterator dir, TVectorType& adj )
    {
        TVectorType r;
        EdsMath::toVectorDiscrete< EdsMath::Angle< 8 > >( 
            static_cast< typename EdsMath::Angle< 8 >::Value >( dir ), r.x, r.y );
        adj = v + r;
    }
};

}


#endif //ADJACENCY_15_09_2013