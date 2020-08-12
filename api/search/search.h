#ifndef BFS_15_09_2013
#define BFS_15_09_2013

#include "common/stl.hpp"

#include <list>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace Search
{
    
enum SearchTermination : unsigned char
{
    eST_Continue,
    eST_Complete,
    eST_Exhausted,
    eST_Maximum,
    TOTAL_SEARCH_TERMINATIONS
};

template< class T >
struct DumbVisitor
{
    static inline float Heuristic_Estimate( const T& vertex )
    {
        //calculate the estimate weight for the vertex
        return 0.0f;
    }
    static inline float Heuristic_Known( const T& fromVertex, const T& toVertex )
    {
        //calculate the known cost between the two vertices
        return 0.0f;
    }
    static inline float Heuristic_Priority( float fKnownWeight, float fHeuristicWeight )
    {
        //compose the two priorities along with any other weights to calculate total weight
        return fKnownWeight + fHeuristicWeight;
    }
    static inline SearchTermination complete( const T& vertex, unsigned int uiCounter )
    {
        //only called once for each vertex
        //return any of SearchTermination to control continuation of search
        return eST_Continue;
    }
    static inline bool filter( const T& currentVertex, const T& candidateVertex )
    {
        //return true to filter out
        return false;
    }
};

template< class T, class TOrder >
class Search
{
public:
    typedef float WeightType;
    typedef std::multimap< WeightType, T >  PriorityQueue;
    typedef std::pair< WeightType, T >      PriorityQueuePair;
    typedef std::set< T, TOrder >           NodeSet;
    typedef std::map< T, WeightType, TOrder >    WeightMap;
    typedef std::map< T, T, TOrder >        PredecessorMap;

private:
    PriorityQueue m_priorityQueue;
    NodeSet m_openSet, m_closedSet;
    WeightMap m_weight;
    PredecessorMap m_predecessors;
    unsigned int m_uiCounter;

public:
    Search()
        :   m_uiCounter( 0u )
    {
    }

    const PredecessorMap& getPredecessors() const { return m_predecessors; }

    bool hasResult( T& result ) const
    {
        bool bHasResult = false;
        if( !m_priorityQueue.empty() )
        {
            PriorityQueue::const_iterator 
                iStart = m_priorityQueue.begin();
            result = iStart->second;
            bHasResult = true;
        }
        return bHasResult;
    }

    void getResult( std::vector< T >& result ) const
    {
        T iter;
        if( hasResult( iter ) )
        {
            int iSanityCheck = 10000;
            while( --iSanityCheck )
            {
                result.push_back( iter );
                PredecessorMap::const_iterator iFind = m_predecessors.find( iter );
                if( iFind == m_predecessors.end() )
                    break;
                else
                    iter = iFind->second;
            }
            if( !iSanityCheck ) throw std::runtime_error( "overrun computing astar search result" );
        }
        std::reverse( result.begin(), result.end() );
    }

    void reset()
    {
        m_uiCounter = 0u;
        m_priorityQueue.clear();
        m_openSet.clear();
        m_closedSet.clear();
        m_weight.clear();
        m_predecessors.clear();
    }

    void initialise( T start )
    {
        reset();
        m_priorityQueue.insert( PriorityQueuePair( 0.0f, start ) );
        m_openSet.insert( start );
        m_weight[ start ] = 0.0f;
    }

    template< class Visitor >
    void initialise( const std::list< std::pair< T, WeightType > >& startList, Visitor& visitor )
    {
        reset();
        for( std::list< std::pair< T, WeightType > >::const_iterator 
            i = startList.begin(), iEnd = startList.end(); i!=iEnd; ++i )
        {
            const WeightType fDist = visitor.Heuristic_Estimate( i->first );
            m_priorityQueue.insert( PriorityQueuePair( i->second + fDist, i->first ) );
            m_openSet.insert( i->first );
            m_weight[ i->first ]  = i->second;
        }
    }

    template< class AdjacencyType, class Visitor >
    SearchTermination search( Visitor& visitor )
    {
        SearchTermination terminationType = eST_Maximum;

        while( !m_priorityQueue.empty() )
        {
            PriorityQueue::iterator iFirst = m_priorityQueue.begin();
            T node_current = iFirst->second;
            const WeightType node_current_weight = m_weight[ node_current ];

            terminationType = visitor.complete( node_current, m_uiCounter++ );
            if( terminationType != eST_Continue )
                break;

            m_priorityQueue.erase( iFirst );
            m_openSet.erase( node_current );
            m_closedSet.insert( node_current );
        
            //open all adjacent nodes
            for( AdjacencyType::Iterator 
                    n = AdjacencyType::Begin( node_current ); 
                    n != AdjacencyType::End( node_current ); 
                    AdjacencyType::Increment( n ) )
            {
                T node_adjacent;
                AdjacencyType::adjacent< T >( node_current, n, node_adjacent );

                if( m_closedSet.find( node_adjacent ) != m_closedSet.end() )
                    continue;

                if( visitor.filter( node_current, node_adjacent ) )
                    continue;
                    
                const bool bNewNode = m_openSet.find( node_adjacent ) == m_openSet.end() ? true : false;
                const WeightType fKnownWeight = node_current_weight + visitor.Heuristic_Known( node_current, node_adjacent );

                if( bNewNode )
                    m_openSet.insert( node_adjacent );

                const WeightType fAdjacentOldWeight = !bNewNode ? m_weight[ node_adjacent ] : 0.0f;
                if( bNewNode || fKnownWeight < fAdjacentOldWeight )
                {
                    m_predecessors[ node_adjacent ] = node_current;
                    m_weight[ node_adjacent ] = fKnownWeight;

                    //update priority queue
                    if( !bNewNode )
                    {
                        for( PriorityQueue::iterator i = m_priorityQueue.begin(),
                            iEnd = m_priorityQueue.end(); i!=iEnd; ++i )
                        {
                            if( i->second == node_adjacent )
                            {
                                m_priorityQueue.erase( i );
                                break;
                            }
                        }
                    }
                    const WeightType fHeuristicWeight = visitor.Heuristic_Estimate( node_adjacent );
                    const WeightType fPriority = visitor.Heuristic_Priority( fKnownWeight, fHeuristicWeight );
                    m_priorityQueue.insert( PriorityQueuePair( fPriority, node_adjacent ) );
                }
            }
        }

        return terminationType;
    }
};

}


#endif //BFS_15_09_2013