#ifndef CONNECTION_07_NOV_2020
#define CONNECTION_07_NOV_2020

#include "wykobi.hpp"

#include "common/angle.hpp"

#include <boost/shared_ptr.hpp>

#include <map>
#include <utility>
#include <vector>

namespace Blueprint
{
    class Area;
    class Feature_ContourSegment;
    
    class ConnectionAnalysis
    {
    public:
        static inline const float fConnectionMaxDist = 4.5f;
        static inline const float fQuantisation      = 5.0f;
        
        struct FCSID
        {
            Math::Angle< 8 >::Value angle;
            int x, y; //quantised
        };
        struct FCSValue
        {
            using Vector = std::vector< FCSValue >;
            using CstPtr = const FCSValue*;
            using CstPtrPair = std::pair< CstPtr, CstPtr >;
            using CstPtrPairVector = std::vector< CstPtrPair >;
            FCSID id;
            const Feature_ContourSegment* pFCS;
            float x, y; //not quantised
        };
        
        using ComponentVector = std::vector< int >;
        using AreaIDMap = std::map< const Area*, int >;
        using AreaIDTable = std::vector< const Area* >;
        
        using ConnectionPair = std::pair< const Feature_ContourSegment*, const Feature_ContourSegment* >;
        
        struct Connection
        {
            using Ptr = std::shared_ptr< Connection >;
            
            Connection( const ConnectionPair& cp );
            
            const wykobi::polygon< float, 2u >& getPolygon() const { return m_polygon; }
        private:
            wykobi::polygon< float, 2u > m_polygon;
        };
        
        using ConnectionPairMap = std::map< ConnectionPair, Connection::Ptr >;
        
        ConnectionAnalysis( const Area& area )
            :   m_area( area )
        {}
        
        const ConnectionPairMap& getConnections() const { return m_connections; }
        const FCSValue::CstPtrPairVector& getFCSPairs() const { return m_fcsPairs; }
        const AreaIDTable& getAreaIDTable() const { return m_areaIDTable; }
        int getTotalComponents() const { return m_totalComponents; }
        const ComponentVector& getComponents() const { return m_components; }
        bool isFeatureContourSegmentConnected( Feature_ContourSegment* pFeatureContourSegment ) const;
        
        void calculate();
        
    private:
        void calculateConnections();
        void calculateConnectedComponents();
        
    private:
        const Area& m_area;
        FCSValue::Vector m_fcs;
        FCSValue::CstPtrPairVector m_fcsPairs;
        ConnectionPairMap m_connections;
        AreaIDMap m_areaIDMap;
        AreaIDTable m_areaIDTable;
        int m_totalComponents;
        ComponentVector m_components;
    };
    
    class ExteriorAnalysis
    {
    public:
        static inline const float fExtrusionAmt = 3.0f;
        
        ExteriorAnalysis( const Area& area, const ConnectionAnalysis& connections )
            :   m_area( area ),
                m_connections( connections )
        {}
        
        void calculate();
        
        struct Exterior
        {
            friend class ExteriorAnalysis;
            
            using Ptr = std::shared_ptr< Exterior >;
            using PtrVector = std::vector< Ptr >;
            using AreaVector = std::vector< const Area* >;
            
            const wykobi::polygon< float, 2u >& getPolygon() const { return m_polygon; }
        private:
            AreaVector m_areas;
            wykobi::polygon< float, 2u > m_polygon;
        };
        
        const Exterior::PtrVector& getExteriors() const { return m_exteriors; }
        
    private:
        const Area& m_area;
        const ConnectionAnalysis& m_connections;
        Exterior::PtrVector m_exteriors;
    };
}

#endif //CONNECTION_07_NOV_2020