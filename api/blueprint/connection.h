#ifndef CONNECTION_07_NOV_2020
#define CONNECTION_07_NOV_2020

#include "wykobi.hpp"

#include <boost/shared_ptr.hpp>

#include <map>
#include <utility>

namespace Blueprint
{
    class Area;
    class Feature_ContourSegment;
    
    class ConnectionAnalysis
    {
    public:
        ConnectionAnalysis( const Area& area )
            :   m_area( area )
        {}
        
        struct Connection
        {
            using Ptr = std::shared_ptr< Connection >;
            
            wykobi::polygon< float, 2u > polygon;
        };
        
        using ConnectionPair = std::pair< const Feature_ContourSegment*, const Feature_ContourSegment* >;
        using ConnectionPairMap = std::map< ConnectionPair, Connection::Ptr >;
        
        const ConnectionPairMap& getConnections() const { return m_connections; }
        
        void calculate();

        bool isFeatureContourSegmentConnected( Feature_ContourSegment* pFeatureContourSegment ) const;
        
    private:
        const Area& m_area;
        ConnectionPairMap m_connections;
    };
    
}

#endif //CONNECTION_07_NOV_2020