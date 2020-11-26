
#include "blueprint/compiler.h"
#include "blueprint/connection.h"

#include "common/angle.hpp"

#include "blueprint/cgalSettings.h"

#include <iostream>

namespace
{
    struct Boundary
    {
        unsigned int edge;
        wykobi::point2d< float > start, end;
        float distance;
        const Blueprint::Feature_ContourSegment* pFCS;
    };
    
    struct ContourPoint
    {
        using Ptr           = std::shared_ptr< ContourPoint >;
        using PtrVector     = std::vector< Ptr >;
        
        using ExteriorConnectionPair = std::pair< Ptr, Ptr >;
        using ExteriorConnectionPairVector = std::vector< ExteriorConnectionPair >;
        
        using Angle8Traits  = Math::Angle< 8 >;
        using Angle8        = Angle8Traits::Value;
        using Point2D       = wykobi::point2d< float >;
        using Segment2D     = wykobi::segment< float, 2 >;
        using Vector2D      = wykobi::vector2d< float >;
        
        friend void getInteriorContourPoints( const Blueprint::Matrix& transform,
                const Blueprint::ConnectionAnalysis& connectionAnalysis, 
                const Blueprint::Area* pArea, 
                const std::vector< Boundary >& boundaries,
                const wykobi::polygon< float, 2 >& contour,
                ContourPoint::PtrVector& points, 
                ContourPoint::ExteriorConnectionPairVector& exteriorConnectionPairs );
                        
        enum PointType
        {
            Normal,
            Start,
            End
        };
        
        ContourPoint( PointType _type, Point2D _pt,
                    const Blueprint::Feature_ContourSegment* _pFCS )
            :   m_type( _type ),
                m_pt( _pt ),
                m_pFCS( _pFCS )
        {
        }
        
        PointType getType() const { return m_type; }
        const Point2D& getPoint() const { return m_pt; }
        const Blueprint::Feature_ContourSegment* getFCS() const { return m_pFCS; }
        
    private:
        PointType m_type;
        Point2D m_pt;
        const Blueprint::Feature_ContourSegment* m_pFCS = nullptr;
    };
    
    void getInteriorBoundaries( const Blueprint::ConnectionAnalysis& connectionAnalysis, 
                        const Blueprint::Area* pArea,
                        std::vector< Boundary >& boundaries )
    {
        for( Blueprint::Feature_ContourSegment::Ptr pBoundaryPtr : pArea->getBoundaries() )
        {
            if( connectionAnalysis.isFeatureContourSegmentConnected( pBoundaryPtr.get() ) || 
                pBoundaryPtr->isSegmentExterior() )
            {
                unsigned int polyIndex = 0; 
                {
                    float fDistance = 0.0f;
                    ContourPoint::Point2D midPoint;
                    pBoundaryPtr->getBoundaryPoint( 
                        Blueprint::Feature_ContourSegment::eMidPoint, 
                        polyIndex, midPoint.x, midPoint.y, fDistance );
                }
                    
                float startfDistance = 0.0f;
                unsigned int startpolyIndex = 0; 
                ContourPoint::Point2D start;
                {
                    pBoundaryPtr->getBoundaryPoint( 
                        Blueprint::Feature_ContourSegment::eLeft, 
                        startpolyIndex, start.x, start.y, startfDistance );
                }
                    
                unsigned int endpolyIndex = 0; 
                ContourPoint::Point2D end;
                {
                    float endfDistance = 0.0f;
                    pBoundaryPtr->getBoundaryPoint( 
                        Blueprint::Feature_ContourSegment::eRight, 
                        endpolyIndex, end.x, end.y, endfDistance );
                }
                    
                const ContourPoint::Point2D offset( 
                    wykobi::make_point< float >( 
                        pArea->getContour()->getX( 0u ), 
                        pArea->getContour()->getY( 0u ) ) );
                    
                VERIFY_RTE_MSG( ( polyIndex == startpolyIndex ) && ( polyIndex == endpolyIndex ), 
                    "Boundary overlaps segment edges" );
                    
                boundaries.push_back( 
                    Boundary
                    { 
                        polyIndex, start + offset, end + offset, startfDistance , pBoundaryPtr.get()
                    } );
            }
        }
    }
    
    void getInteriorContourPoints( const Blueprint::Matrix& transform,
            const Blueprint::ConnectionAnalysis& connectionAnalysis, 
            const Blueprint::Area* pArea, 
            const std::vector< Boundary >& boundaries,
            const wykobi::polygon< float, 2 >& contour,
            ContourPoint::PtrVector& points, 
            ContourPoint::ExteriorConnectionPairVector& exteriorConnectionPairs )
    {
        ContourPoint::ExteriorConnectionPairVector newExteriorConnections;
        {
            int totalBoundaries = 0;
            unsigned int iEdgeCounter = 0;
            for( wykobi::polygon< float, 2 >::const_iterator 
                i = contour.begin(),
                iEnd = contour.end(); i!=iEnd; ++i, ++iEdgeCounter )
            {
                points.push_back( ContourPoint::Ptr( new ContourPoint( ContourPoint::Normal, *i, nullptr ) ) );
                
                std::map< float, const Boundary* > inSegmentBoundaries;
                for( const Boundary& boundary : boundaries )
                {
                    if( boundary.edge == iEdgeCounter )
                    {
                        auto result = 
                            inSegmentBoundaries.insert( std::make_pair( boundary.distance, &boundary ) );
                        VERIFY_RTE_MSG( result.second, "Duplicate segment distance error" );
                    }
                }
                
                for( const auto& boundPair : inSegmentBoundaries )
                {
                    const std::size_t sz = points.size();
                    points.push_back( ContourPoint::Ptr( 
                        new ContourPoint( ContourPoint::Start,  boundPair.second->start, boundPair.second->pFCS ) ) );
                    points.push_back( ContourPoint::Ptr( 
                        new ContourPoint( ContourPoint::End,    boundPair.second->end  , boundPair.second->pFCS ) ) );
                    ++totalBoundaries;
                    
                    if( boundPair.second->pFCS->isSegmentExterior() )
                    {
                        newExteriorConnections.push_back(
                            std::make_pair( points[ sz ], points[ sz + 1U ] ) );
                    }
                }
            }
            VERIFY_RTE_MSG( totalBoundaries == boundaries.size(), "Boundary error" );
        }
        
        //calculate absolute points
        {
            for( ContourPoint::Ptr pPoint : points )
            {
                transform.transform( pPoint->m_pt.x, pPoint->m_pt.y );
            }
        }
        
        //check final orientation
        bool bFlipped = false;
        {
            wykobi::polygon< float, 2 > completeContour;
            for( ContourPoint::Ptr pPoint : points )
            {
                completeContour.push_back( pPoint->m_pt );
            }
            const int polyOrientation = wykobi::polygon_orientation( completeContour );
            if( polyOrientation == wykobi::CounterClockwise )
            {
                bFlipped = true;
                std::reverse( points.begin(), points.end() );
            }
        }
        
        //capture exterior connection pairs
        {
            for( auto& p : newExteriorConnections )
            {
                if( bFlipped )
                {
                    auto t = p;
                    p.first = t.second;
                    p.second = t.first;
                }
                exteriorConnectionPairs.push_back( p );
            }
        }
    }
    
    void getExteriorBoundaries( const Blueprint::Matrix& transform,
        const wykobi::polygon< float, 2u >& exteriorPolygon,
        const ContourPoint::ExteriorConnectionPairVector& exteriorConnectionPairs,
        std::vector< Boundary >& boundaries )
    {
        unsigned int szEdgeIndex = 0;
        for( wykobi::polygon< float, 2u >::const_iterator
            i     = exteriorPolygon.begin(),
            iNext = exteriorPolygon.begin(),
            iEnd  = exteriorPolygon.end();
            i != iEnd;
            ++i, ++szEdgeIndex )
        {
            ++iNext;
            if( iNext == iEnd ) 
                iNext = exteriorPolygon.begin();
            
            const Math::Angle< 8 >::Value exteriorAngle = 
                Math::fromVector< Math::Angle< 8 > >( iNext->x - i->x, iNext->y - i->y );
                
            const ContourPoint::Segment2D exteriorLine = wykobi::make_segment( *i, *iNext );
            
            for( const ContourPoint::ExteriorConnectionPair& ecp : exteriorConnectionPairs )
            {
                ContourPoint::Ptr pStart  = ecp.first;
                ContourPoint::Ptr pEnd    = ecp.second;
                
                //determine if exterior line segment is valid for interior connection
                const Math::Angle< 8 >::Value interiorAngle = 
                    Math::fromVector< Math::Angle< 8 > >( 
                        pEnd->getPoint().x - pStart->getPoint().x, 
                        pEnd->getPoint().y - pStart->getPoint().y );
                        
                //outer contour is clockwise - interior is counter-clockwise - so these two angles
                //just need to be equal for them to be in opposing direction.
                if( interiorAngle == exteriorAngle )
                {
                    const Math::Angle< 8 >::Value orthoNormal =
                        static_cast< Math::Angle< 8 >::Value >( ( interiorAngle + 2 ) % 8 );
                    ContourPoint::Vector2D vDir;
                    Math::toVector< Math::Angle< 8 > >( orthoNormal, vDir.x, vDir.y );
                    
                    const ContourPoint::Segment2D connectionStartLine = wykobi::make_segment( 
                            pStart->getPoint(), 
                            pStart->getPoint() + vDir * Blueprint::ConnectionAnalysis::fConnectionMaxDist );
                        
                    const ContourPoint::Segment2D connectionEndLine = wykobi::make_segment( 
                            pEnd->getPoint(), 
                            pEnd->getPoint() + vDir * Blueprint::ConnectionAnalysis::fConnectionMaxDist );
                        
                    if( intersect( exteriorLine, connectionStartLine ) &&
                        intersect( exteriorLine, connectionEndLine ) )
                    {
                        const ContourPoint::Point2D ptStart =
                            intersection_point( exteriorLine, connectionStartLine );
                        const ContourPoint::Point2D ptEnd =
                            intersection_point( exteriorLine, connectionEndLine );
                            
                        const float startfDistance = wykobi::distance( pStart->getPoint(), ptStart );
                            
                        std::cout << "Exterior boundary at: (" << ptStart.x << "," << ptStart.y << ") (" << 
                            ptEnd.x << "," << ptEnd.y << ") distance:" << startfDistance << std::endl;
                        boundaries.push_back(
                            Boundary
                            { 
                                szEdgeIndex, ptStart, ptEnd, startfDistance, pStart->getFCS()
                            } );
                    }
                }
            }
        }
    }
    
    void getExteriorContourPoints( 
            const Blueprint::ConnectionAnalysis& connectionAnalysis, 
            const Blueprint::Area* pArea, 
            const std::vector< Boundary >& boundaries,
            const wykobi::polygon< float, 2 >& contour,
            ContourPoint::PtrVector& points )
    {
        ContourPoint::ExteriorConnectionPairVector newExteriorConnections;
        {
            int totalBoundaries = 0;
            unsigned int iEdgeCounter = 0;
            for( wykobi::polygon< float, 2 >::const_iterator 
                i = contour.begin(),
                iEnd = contour.end(); i!=iEnd; ++i, ++iEdgeCounter )
            {
                points.push_back( ContourPoint::Ptr( new ContourPoint( ContourPoint::Normal, *i, nullptr ) ) );
                
                std::map< float, const Boundary* > inSegmentBoundaries;
                for( const Boundary& boundary : boundaries )
                {
                    if( boundary.edge == iEdgeCounter )
                    {
                        auto result = 
                            inSegmentBoundaries.insert( std::make_pair( boundary.distance, &boundary ) );
                        VERIFY_RTE_MSG( result.second, "Duplicate segment distance error" );
                    }
                }
                
                for( const auto& boundPair : inSegmentBoundaries )
                {
                    const std::size_t sz = points.size();
                    points.push_back( ContourPoint::Ptr( 
                        new ContourPoint( ContourPoint::Start,  boundPair.second->start, boundPair.second->pFCS ) ) );
                    points.push_back( ContourPoint::Ptr( 
                        new ContourPoint( ContourPoint::End,    boundPair.second->end  , boundPair.second->pFCS ) ) );
                    ++totalBoundaries;
                }
            }
            VERIFY_RTE_MSG( totalBoundaries == boundaries.size(), "Boundary error" );
        }
        
        //check final orientation
        bool bFlipped = false;
        {
            wykobi::polygon< float, 2 > completeContour;
            for( ContourPoint::Ptr pPoint : points )
            {
                completeContour.push_back( pPoint->getPoint() );
            }
            const int polyOrientation = wykobi::polygon_orientation( completeContour );
            if( polyOrientation == wykobi::Clockwise )
            {
                bFlipped = true;
                std::reverse( points.begin(), points.end() );
            }
        }
        
    }
    
    bool getAbsoluteOrientedContour( const Blueprint::Matrix& transform, int iOrientation, wykobi::polygon< float, 2u >& contour )
    {
        for( ContourPoint::Point2D& pt : contour )
        {
            transform.transform( pt.x, pt.y );
        }
        const int polyOrientation = wykobi::polygon_orientation( contour );
        if( polyOrientation != iOrientation )
        {
            std::reverse( contour.begin(), contour.end() );
            return true;
        }
        else
        {
            return false;
        }
    }
}

namespace Blueprint
{
    
class Compiler::CompilerImpl
{
    struct AreaInfo
    {
        using Ptr = std::shared_ptr< AreaInfo >;
        using PtrMap = std::map< const Area*, Ptr >;
        
        const Area* pArea;
        std::vector< Boundary > boundaries;
        ContourPoint::PtrVector interiorPoints;
    };
    AreaInfo::PtrMap m_areaInfoMap;
    
    struct ExteriorInfo
    {
        using Ptr = std::shared_ptr< ExteriorInfo >;
        using PtrMap = std::map< const ExteriorAnalysis::Exterior*, Ptr >;
        
        const Area* pArea;
        ExteriorAnalysis::Exterior* pExterior;
        std::vector< Boundary > boundaries;
        ContourPoint::PtrVector exteriorPoints;
    };
    ExteriorInfo::PtrMap m_exteriorInfoMap;
    
    void recurse( Matrix transform, const Area* pArea )
    {
        pArea->getTransform().transform( transform );
        
        const ConnectionAnalysis& connections = pArea->getConnections();
        
        ContourPoint::ExteriorConnectionPairVector exteriorConnectionPairs;
        
        //first process all child area interiors
        for( Site::Ptr pSite : pArea->getSpaces() )
        {
            if( const Area* pChildArea = dynamic_cast< const Area* >( pSite.get() ) )
            {
                Matrix childTransform = transform;
                pChildArea->getTransform().transform( childTransform );
                
                AreaInfo::Ptr pAreaInfo( new AreaInfo{ pChildArea } );
                
                //get the relative boundary info
                getInteriorBoundaries( connections, pChildArea, pAreaInfo->boundaries ); 
                    
                const wykobi::polygon< float, 2 > contour = pChildArea->getContour()->getPolygon();
                    
                //using the relative contour and boundaries produce the absolute contour with correct winding
                getInteriorContourPoints( childTransform, connections, pChildArea, pAreaInfo->boundaries, contour, 
                    pAreaInfo->interiorPoints, exteriorConnectionPairs );
                
                m_areaInfoMap.insert( std::make_pair( pChildArea, pAreaInfo ) );
            }
        }
        
        //now construct exterior points WITH connection points
        const ExteriorAnalysis& exteriorAnalysis = pArea->getExteriors();
        const ExteriorAnalysis::Exterior::PtrVector& exteriors = exteriorAnalysis.getExteriors();
        
        for( ExteriorAnalysis::Exterior::Ptr pExterior : exteriors )
        {
            ExteriorInfo::Ptr pExteriorInfo( new ExteriorInfo );
            {
                pExteriorInfo->pArea = pArea;
                pExteriorInfo->pExterior = pExterior.get();
            }
            m_exteriorInfoMap.insert( std::make_pair( pExterior.get(), pExteriorInfo ) );
            
            //get the absolute contour
            wykobi::polygon< float, 2u > exteriorPolygon = pExterior->getPolygon();
            getAbsoluteOrientedContour( transform, wykobi::Clockwise, exteriorPolygon );
            
            //determine absolute boundaries
            getExteriorBoundaries( transform, exteriorPolygon, exteriorConnectionPairs, pExteriorInfo->boundaries );
                
            //produce the absolute exterior contour
            getExteriorContourPoints( connections, pArea, pExteriorInfo->boundaries, exteriorPolygon,
                pExteriorInfo->exteriorPoints );
            
        }
        
        for( Site::Ptr pSite : pArea->getSpaces() )
        {
            if( const Area* pChildArea = dynamic_cast< const Area* >( pSite.get() ) )
            {
                recurse( transform, pChildArea );
            }
        }
    }

public:
    CompilerImpl( const Site::PtrVector& sites )
        :   m_sites( sites )
    {
        //ensure evaluated
        for( Site::Ptr pSite : sites )
        {
            if( Area* pChildArea = dynamic_cast< Area* >( pSite.get() ) )
            {
                Site::EvaluationMode mode;
                DataBitmap data;
                pChildArea->evaluate( mode, data );
            }
        }
        
        Matrix transform;
        for( Site::Ptr pSite : sites )
        {
            if( Area* pChildArea = dynamic_cast< Area* >( pSite.get() ) )
            {
                recurse( transform, pChildArea );
            }
        }
    }
  
    const Site::PtrVector& m_sites;
    Arr_with_hist_2 m_arr;
};

Compiler::Compiler( const Site::PtrVector& sites )
    :   m_pPimpl( std::make_shared< CompilerImpl >( sites ) )
{
}

}
