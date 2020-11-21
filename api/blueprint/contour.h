

#ifndef CONTOUR_ANALYSIS_20_NOV_2020
#define CONTOUR_ANALYSIS_20_NOV_2020

#include "blueprint/blueprint.h"
#include "blueprint/basicarea.h"

#include "wykobi.hpp"

#include "common/angle.hpp"

#include <optional>
#include <string>
#include <cmath>
#include <tuple>

namespace Blueprint
{
    using Angle8Traits = Math::Angle< 8 >;
    using Angle8 = Angle8Traits::Value;
    using Point2D = wykobi::point2d< float >;
    using Polygon2D = wykobi::polygon< float, 2 >;
    
    const float fMaxConnectionLength = 4.5f;
    const float fWallLength = 10.0f;
    const float fWallLengthBy2 = 5.0f;
    
    struct ConnectionAnalysis
    {
        using FeatureContourSegmentPair = std::pair< Feature_ContourSegment::Ptr, Feature_ContourSegment::Ptr >;
        using FeatureContourSegmentPairVector = std::vector< FeatureContourSegmentPair >;
        
        ConnectionAnalysis( Blueprint::Ptr pBlueprint );
        
        bool isFeatureContourSegmentConnected( Feature_ContourSegment::Ptr pFeatureContourSegment ) const;
    private:
        FeatureContourSegmentPairVector m_segmentPairs;
    };
    
    struct Point
    {
        friend void getAreaPoints( const boost::filesystem::path& errorPath, 
                    const ConnectionAnalysis& connectionAnalysis, Area::Ptr pArea,
                    std::vector< Point >& points, int& totalBoundaries );
                    
        using RawCstPtr = const Point*;
        using RawCstPtrVector = std::vector< RawCstPtr >;
        
        enum PointType
        {
            Normal,
            Start,
            End
        };
        
        Point( PointType _type, Point2D _pt );
        Point( PointType _type, Point2D _pt, Angle8 _angle );
        
        PointType getType() const { return m_type; }
        const Point2D& getRelPoint() const { return m_pt; }
        const Point2D& getAbsPoint() const { return m_absolutePt; }
        std::optional< Angle8 > getAngle() const { return m_directionOpt; }
        
        void setAngle( Angle8 angle ) const { m_directionOpt = angle; }
        
    private:
        PointType m_type;
        Point2D m_pt, m_absolutePt;
        mutable std::optional< Angle8 > m_directionOpt;
    };
    
    void getAreaPoints( const boost::filesystem::path& errorPath, 
        const ConnectionAnalysis& connectionAnalysis, Area::Ptr pArea,
        std::vector< Point >& points, int& totalBoundaries );
    
    struct WallSection
    {
        using Vector            = std::vector< WallSection >;
        using RawCstPtr         = const WallSection*;
        using List              = std::list< RawCstPtr >;
        using ListPtr           = std::shared_ptr< List >;
        using ListPtrVector     = std::vector< ListPtr >;
        
        WallSection( int index, bool bClosed, int iOrientation )
            :   m_index( index ),
                m_bClosed( bClosed ),
                m_polyOrientation( iOrientation )
        {
        }
        
        void push_back_point( const Point* pPoint )
        {
            m_points.push_back( pPoint );
        }
        void set_last_point( const Point* pPoint )
        {
            VERIFY_RTE( !m_points.empty() );
            m_points.back() = pPoint;
        }
        const Point* WallSection::getPoint( std::size_t szIndex ) const
        {
            VERIFY_RTE( szIndex < m_points.size() );
            return m_points[ szIndex ];
        }
        
        //for use after construction - in terms of absolute points
        bool isClosed() const { return m_bClosed; }
        std::size_t size() const { return m_points.size(); }
        const Point* front() const;
        const Point* back() const;
        const Point* get( std::size_t szIndex ) const;
        void getAbsPoints( Polygon2D& points ) const;
    private:
        int m_index; //counter clockwise index
        bool m_bClosed; //is this section a complete polygon
        Point::RawCstPtrVector m_points;
        int m_polyOrientation;
    };
    
    void getWallSections( const boost::filesystem::path& errorPath, const std::vector< Point >& points, int totalBoundaries, 
        std::vector< WallSection >& wallSections );
        
    void getWallSectionLists( const WallSection::Vector& wallSections, 
        WallSection::ListPtrVector& wallSectionLists );
        
    struct Contour
    {
        using Ptr = std::shared_ptr< Contour >;
        using PtrVector = std::vector< Ptr >;
        
        friend void getContours( const WallSection::ListPtrVector& wallSectionLists, Contour::PtrVector& contours );
        
        WallSection::ListPtr getOuter() const { return m_pOuter; }
        WallSection::ListPtrVector getHoles() const { return m_holes; }
        
        void getOuterPolygon( Polygon2D& polygon ) const;
        
    private:
        WallSection::ListPtr m_pOuter;
        WallSection::ListPtrVector m_holes;
    };
        
    void getContours( const WallSection::ListPtrVector& wallSectionLists, Contour::PtrVector& contours );
}



#endif //CONTOUR_ANALYSIS_20_NOV_2020