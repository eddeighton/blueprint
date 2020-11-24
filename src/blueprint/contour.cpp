
#include "blueprint/contour.h"

#include "clipper.hpp"

namespace Blueprint
{



Point::Point( PointType _type, Point2D _pt )
    :   m_type( _type ),
        m_pt( _pt )
{
}

Point::Point( PointType _type, Point2D _pt, Angle8 _angle )
    :   m_type( _type ),
        m_pt( _pt ),
        m_directionOpt( _angle )
{
}

void getAreaPoints( const boost::filesystem::path& errorPath, 
        const ConnectionAnalysis& connectionAnalysis, Area::Ptr pArea, 
        std::vector< Point >& points, int& totalBoundaries )
{
    struct Boundary
    {
        unsigned int edge;
        Point2D start, end;
        float distance;
    };
    std::vector< Boundary > boundaries;
    
    for( Feature_ContourSegment::Ptr pBoundaryPtr : pArea->getBoundaries() )
    {
        if( connectionAnalysis.isFeatureContourSegmentConnected( pBoundaryPtr.get() ) )
        {
            unsigned int polyIndex = 0; 
            {
                float fDistance = 0.0f;
                Point2D midPoint;
                pBoundaryPtr->getBoundaryPoint( 
                    Feature_ContourSegment::eMidPoint, 
                    polyIndex, midPoint.x, midPoint.y, fDistance );
            }
                
            float startfDistance = 0.0f;
            unsigned int startpolyIndex = 0; 
            Point2D start;
            {
                pBoundaryPtr->getBoundaryPoint( 
                    Feature_ContourSegment::eLeft, 
                    startpolyIndex, start.x, start.y, startfDistance );
            }
                
            unsigned int endpolyIndex = 0; 
            Point2D end;
            {
                float endfDistance = 0.0f;
                pBoundaryPtr->getBoundaryPoint( 
                    Feature_ContourSegment::eRight, 
                    endpolyIndex, end.x, end.y, endfDistance );
            }
                
            const Point2D offset( 
                wykobi::make_point< float >( 
                    pArea->getContour()->getX( 0u ), 
                    pArea->getContour()->getY( 0u ) ) );
                
            VERIFY_RTE_MSG( ( polyIndex == startpolyIndex ) && ( polyIndex == endpolyIndex ), 
                "Boundary overlaps segment edges: " << errorPath.string() );
                
            boundaries.push_back( 
                Boundary
                { 
                    polyIndex, start + offset, end + offset, startfDistance 
                } );
        }
    }
    
    {
        const wykobi::polygon< float, 2 >& areaPolygon = pArea->getContour()->getPolygon();
        
        {
            const int polyOrientation = wykobi::polygon_orientation( areaPolygon );
            VERIFY_RTE_MSG( polyOrientation == wykobi::CounterClockwise, "Polygon orientation: " << polyOrientation << " for: " << errorPath.string() );
        }
        
        unsigned int iEdgeCounter = 0;
        for( wykobi::polygon< float, 2 >::const_iterator 
            i = areaPolygon.begin(),
            iEnd = areaPolygon.end(); i!=iEnd; ++i, ++iEdgeCounter )
        {
            points.push_back( Point( Point::Normal, *i ) );
            
            std::map< float, const Boundary* > inSegmentBoundaries;
            for( const Boundary& boundary : boundaries )
            {
                if( boundary.edge == iEdgeCounter )
                {
                    auto result = 
                        inSegmentBoundaries.insert( std::make_pair( boundary.distance, &boundary ) );
                    VERIFY_RTE_MSG( result.second, "Duplicate boundary distance error in: " << errorPath.string() );
                }
            }
            
            for( const auto& boundPair : inSegmentBoundaries )
            {
                const Angle8 direction = Math::fromVector< Angle8Traits >( 
                    boundPair.second->end - boundPair.second->start );
                
                points.push_back( Point( Point::Start,  boundPair.second->start ) );
                points.push_back( Point( Point::End,    boundPair.second->end, direction ) );
                
                ++totalBoundaries;
            }
        }
        VERIFY_RTE_MSG( totalBoundaries == boundaries.size(), "Boundary error in: " << errorPath.string() );
    }
    
    //check final orientation
    {
        Polygon2D completeContour;
        for( Point& pt : points )
        {
            completeContour.push_back( pt.m_pt );
        }
        const int polyOrientation = wykobi::polygon_orientation( completeContour );
        VERIFY_RTE_MSG( polyOrientation == wykobi::CounterClockwise, "Final Polygon orientation: " << polyOrientation << " for: " << errorPath.string() );
        VERIFY_RTE_MSG( completeContour.size() > 2U, "Final Polygon contains less then 3 points for: " << errorPath.string() );
    }
    
    //calculate absolute points
    {
        Matrix matrix;
        pArea->getAbsoluteTransform( matrix );
        
        //convert points to absolute using area transform
        for( Point& pt : points )
        {
            pt.m_absolutePt = pt.m_pt;
            matrix.transform( pt.m_absolutePt.x, pt.m_absolutePt.y );
        }
    }
}

void WallSection::getAbsPoints( Polygon2D& points ) const
{
    if( m_polyOrientation == wykobi::Clockwise )
    {
        for( Point::RawCstPtrVector::const_reverse_iterator i = m_points.rbegin(),
            iEnd = m_points.rend(); i!=iEnd; ++i )
        {
            points.push_back( (*i)->getAbsPoint() );
        }
    }
    else if( m_polyOrientation == wykobi::CounterClockwise )
    {
        for( Point::RawCstPtrVector::const_iterator i = m_points.begin(),
            iEnd = m_points.end(); i!=iEnd; ++i )
        {
            points.push_back( (*i)->getAbsPoint() );
        }
    }
    else
    {
        THROW_RTE( "Bad orientation of polygon" );
    }
}
        
const Point* WallSection::front() const
{
    VERIFY_RTE( !m_points.empty() );
    
    if( m_polyOrientation == wykobi::Clockwise )
    {
        return m_points.back();
    }
    else if( m_polyOrientation == wykobi::CounterClockwise )
    {
        return m_points.front();
    }
    else
    {
        THROW_RTE( "Bad orientation of polygon" );
    }
}

const Point* WallSection::back() const
{
    VERIFY_RTE( !m_points.empty() );
    
    if( m_polyOrientation == wykobi::Clockwise )
    {
        return m_points.front();
    }
    else if( m_polyOrientation == wykobi::CounterClockwise )
    {
        return m_points.back();
    }
    else
    {
        THROW_RTE( "Bad orientation of polygon" );
    }
}

void getWallSections( const boost::filesystem::path& errorPath, const std::vector< Point >& points, int totalBoundaries, 
    std::vector< WallSection >& wallSections )
{
    int polyOrientation = 0;
    {
        Polygon2D absContour;
        for( const Point& pt : points )
        {
            absContour.push_back( pt.getAbsPoint() );
        }
        
        polyOrientation = wykobi::polygon_orientation( absContour );
    }
    
    if( totalBoundaries == 0 )
    {
        wallSections.push_back( WallSection( 0, true, polyOrientation ) );
        bool bFirst = true;
        const Point* ptLast = nullptr;
        for( const Point& pt : points )
        {
            if( bFirst )
            {
                bFirst = false;
                const Point& last = points.back();
                pt.setAngle( Math::fromVector< Angle8Traits >( pt.getRelPoint() - last.getRelPoint() ) );
            }
            else
            {
                pt.setAngle( Math::fromVector< Angle8Traits >( pt.getRelPoint() - ptLast->getRelPoint() ) );
            }
            wallSections.back().push_back_point( &pt );
            ptLast = &pt;
        }
    }
    else
    {
        std::vector< Point >::const_iterator 
                    i       = points.begin(),
                    iBegin  = points.begin(),
                    iEnd    = points.end();
        for( int iBoundary = 0; iBoundary != totalBoundaries; ++iBoundary )
        {
            bool bFoundStart = false;
            for( ; ; ++i )
            {
                if( i == iEnd ) 
                    i = iBegin;
                const Point& pt = *i;
                
                if( pt.getType() == Point::End )
                {
                    if( !bFoundStart )
                    {
                        bFoundStart = true;
                        wallSections.push_back( WallSection( iBoundary, false, polyOrientation ) );
                        wallSections.back().push_back_point( &pt );
                    }
                    else
                    {
                        THROW_RTE( "Error in point sequence for: " << errorPath.string() );
                    }
                }
                else if( pt.getType() == Point::Start )
                {
                    if( bFoundStart )
                    {
                        const Point* pPrevious = wallSections.back().back();
                        if( !wykobi::is_equal( pPrevious->getRelPoint(), pt.getRelPoint() ) )
                        {
                            pt.setAngle( Math::fromVector< Angle8Traits >( pt.getRelPoint() - pPrevious->getRelPoint() ) );
                             
                            if( ( pPrevious->getAngle().value() != pt.getAngle().value() ) || 
                                ( pPrevious->getType() != Point::Normal ) )
                            {
                                wallSections.back().push_back_point( &pt );
                            }
                            else
                            {
                                wallSections.back().set_last_point( &pt );
                            }
                        }
                        else
                        {
                            pt.setAngle( pPrevious->getAngle().value() );
                            wallSections.back().set_last_point( &pt );
                        }
                        
                        ++i;
                        break;
                    }
                }
                else if( pt.getType() == Point::Normal )
                {
                    if( bFoundStart )
                    {
                        const Point* pPrevious = wallSections.back().back();
                        if( !wykobi::is_equal( pPrevious->getRelPoint(), pt.getRelPoint() ) )
                        {
                            pt.setAngle( Math::fromVector< Angle8Traits >( pt.getRelPoint() - pPrevious->getRelPoint() ) );
                                
                            if( ( pPrevious->getAngle().value() != pt.getAngle().value() ) || 
                                ( pPrevious->getType() != Point::Normal ) )
                            {
                                wallSections.back().push_back_point( &pt );
                            }
                            else
                            {
                                wallSections.back().set_last_point( &pt );
                            }
                        }
                    }
                }
                else
                {
                    THROW_RTE( "Invalid point type" );
                }
            }
        }
    }

    //check points
    {
        for( const WallSection& section : wallSections )
        {
            const std::size_t szSize = section.size();
            for( std::size_t sz = 0U; sz != szSize; ++sz )
            {
                const Point* p = section.getPoint( sz );
                VERIFY_RTE_MSG( p->getAngle().has_value(), 
                    "Failed to set direction for point in: " << errorPath.string() );
            }
        }
    }
}


void getWallSectionLists( const WallSection::Vector& wallSections, 
        WallSection::ListPtrVector& wallSectionLists )
{
    struct AreListsConnected
    {
        bool operator()( WallSection::RawCstPtr pFirst, WallSection::RawCstPtr pSecond ) const
        {
            if( pFirst == pSecond ) 
                return false;
            const Point* pEndOfFirst    = pFirst->back();
            const Point* pStartOfSecond = pSecond->front();
            return wykobi::distance( pEndOfFirst->getAbsPoint(), pStartOfSecond->getAbsPoint() ) < fMaxConnectionLength;
        }
    };
    AreListsConnected testLists;
    
    std::list< WallSection::RawCstPtr > remainingWallSections;
    for( const WallSection& wallSection : wallSections )
    {
        remainingWallSections.push_back( &wallSection );
    }
    
    while( !remainingWallSections.empty() )
    {
        WallSection::ListPtr pList( new WallSection::List );
        {
            WallSection::RawCstPtr pWallSection = remainingWallSections.front();
            pList->push_back( pWallSection );
            remainingWallSections.pop_front();
            wallSectionLists.push_back( pList );
            
            if( pWallSection->isClosed() )
                continue;
        }
        
        //construct the list until the tail meets the end
        while( true )
        {
            WallSection::RawCstPtr pListHead = pList->front();
            WallSection::RawCstPtr pListTail = pList->back();
            
            if( testLists( pListTail, pListHead ) )
            {
                break;
            }
            
            VERIFY_RTE( !remainingWallSections.empty() );
            
            bool bProgress = false;
            for( WallSection::List::iterator 
                i = remainingWallSections.begin();
                i != remainingWallSections.end(); ++i )
            {
                WallSection::RawCstPtr pIter = *i;
                if( pIter->isClosed() )
                    continue;
                
                //attempt to join wall section to list
                
                if( testLists( pListTail, pIter ) )
                {
                    pList->push_back( pIter );
                    remainingWallSections.erase( i );
                    bProgress = true;
                    break;
                }
                else if( testLists( pIter, pListHead ) )
                {
                    pList->push_front( pIter );
                    remainingWallSections.erase( i );
                    bProgress = true;
                    break;
                }
            }
            VERIFY_RTE_MSG( bProgress, "Failed to make progress" );
        }
    }
}

void getContours( const WallSection::ListPtrVector& wallSectionLists, Contour::PtrVector& contours )
{
    using PolyVector = std::vector< Polygon2D >;
    PolyVector polygons;
    
    for( WallSection::ListPtr pList : wallSectionLists )
    {
        Polygon2D polygon;
        for( WallSection::RawCstPtr pWallSection : *pList )
        {
            pWallSection->getAbsPoints( polygon );
        }
        polygons.push_back( polygon );
    }
    
    //determine outer polygons from inner brute force...
    using PolyMap = std::multimap< const Polygon2D*, const Polygon2D* >;
    PolyMap polyMap;
    
    using PolySet = std::set< const Polygon2D* >;
    PolySet remaining, inner;
    for( const Polygon2D& poly : polygons )
        remaining.insert( &poly );
    
    for( std::size_t i = 0; i < polygons.size() - 1; ++i )
    {
        for( std::size_t j = i + 1; j < polygons.size(); ++j )
        {
            const Polygon2D& left = polygons[ i ];
            const Polygon2D& right = polygons[ j ];
            
            const bool bLeftInRight = wykobi::point_in_polygon( left.front(), right );
            const bool bRightInLeft = wykobi::point_in_polygon( right.front(), left );
            
            if( bLeftInRight && bRightInLeft )
            {
                THROW_RTE( "Overlapping polygons" );
            }
            
            if( bRightInLeft )
            {
                polyMap.insert( std::make_pair( &left, &right ) );
                
                remaining.erase( &left );
                remaining.erase( &right );
                auto r = inner.insert( &right );
                VERIFY_RTE( r.second );
                
            }
            else if( bLeftInRight )
            {
                polyMap.insert( std::make_pair( &right, &left ) );
                
                remaining.erase( &left );
                remaining.erase( &right );
                auto r = inner.insert( &left );
                VERIFY_RTE( r.second );
            }
        }
    }
    
    for( const Polygon2D* pRemain : remaining )
    {
        Contour::Ptr pContour( new Contour );
        contours.push_back( pContour );
        const std::size_t szIndex = std::distance( (const Polygon2D*)&polygons[ 0 ], pRemain );
        VERIFY_RTE( szIndex < wallSectionLists.size() );
        pContour->m_pOuter = wallSectionLists[ szIndex ];
    }
    
    for( PolyMap::const_iterator 
            i = polyMap.begin(),
            iEnd = polyMap.end(); i!=iEnd; )
    {
        Contour::Ptr pContour( new Contour );
        contours.push_back( pContour );
        
        const Polygon2D* pOuter = i->first;
        {
            const std::size_t szIndex = std::distance( (const Polygon2D*)&polygons[ 0 ], pOuter );
            VERIFY_RTE( szIndex < wallSectionLists.size() );
            pContour->m_pOuter = wallSectionLists[ szIndex ];
        }
        
        for( ; i->first == pOuter; ++i )
        {
            const Polygon2D* pHole = i->second;
            const std::size_t szIndex = std::distance( (const Polygon2D*)&polygons[ 0 ], pHole );
            VERIFY_RTE( szIndex < wallSectionLists.size() );
            pContour->m_holes.push_back( wallSectionLists[ szIndex ] );
        }
    }
    
}

void Contour::getOuterPolygon( Polygon2D& polygon ) const
{
    for( WallSection::RawCstPtr pWallSection : *m_pOuter )
    {
        pWallSection->getAbsPoints( polygon );
    }
}
        
}