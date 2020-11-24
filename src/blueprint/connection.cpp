

#include "blueprint/connection.h"
#include "blueprint/basicArea.h"
#include "blueprint/contour.h"

#include "common/assert_verify.hpp"

#include "wykobi_algorithm.hpp"

#include <algorithm>

namespace Blueprint
{

void getSegmentPoints( const Feature_ContourSegment* pFCS, Point2D& pLeft, Point2D& pRight )
{
    Feature_Contour::Ptr pContour = boost::dynamic_pointer_cast< Feature_Contour >(
        pFCS->Node::getParent() );
    Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( 
        pContour->Node::getParent() );
    if( pArea->getTransform().isWindingInverted() )
    {
        pLeft = wykobi::make_point< float >
        (
            pFCS->getX( Feature_ContourSegment::eRight ),
            pFCS->getY( Feature_ContourSegment::eRight )
        );
        pRight = wykobi::make_point< float >
        (
            pFCS->getX( Feature_ContourSegment::eLeft ),
            pFCS->getY( Feature_ContourSegment::eLeft )
        );
    }
    else
    {
        pLeft = wykobi::make_point< float >
        (
            pFCS->getX( Feature_ContourSegment::eLeft ),
            pFCS->getY( Feature_ContourSegment::eLeft )
        );
        pRight = wykobi::make_point< float >
        (
            pFCS->getX( Feature_ContourSegment::eRight ),
            pFCS->getY( Feature_ContourSegment::eRight )
        );
    }
    {
        pLeft.x    += pContour->getX( 0 );
        pLeft.y    += pContour->getY( 0 );
        pRight.x   += pContour->getX( 0 );
        pRight.y   += pContour->getY( 0 );
        {
            pArea->getTransform().transform( pLeft.x,  pLeft.y );
            pArea->getTransform().transform( pRight.x, pRight.y );
        }
    }
}

ConnectionAnalysis::Connection::Connection( const ConnectionAnalysis::ConnectionPair& cp )
{
    m_polygon.reserve( 4 );
    
    {
        Point2D pLeft, pRight;
        getSegmentPoints( cp.first, pLeft, pRight );
        
        m_polygon.push_back( pLeft );
        m_polygon.push_back( pRight );
    }
        
    {
        Point2D pLeft, pRight;
        getSegmentPoints( cp.second, pLeft, pRight );
        
        m_polygon.push_back( pLeft );
        m_polygon.push_back( pRight );
    }
    
}
        
void ConnectionAnalysis::calculate()
{
    m_connections.clear();
    
    static const float fConnectionMaxDist   = 4.5f;
    static const float fQuantisation        = 5.0f;
    struct FCSID
    {
        Math::Angle< 8 >::Value angle;
        int x, y; //quantised
    };
    struct FCSValue
    {
        FCSID id;
        const Feature_ContourSegment* pFCS;
        float x, y;
    };
    
    struct CompareFCSID
    {
        inline bool operator()( const FCSID& left, const FCSID& right ) const
        {
            return ( left.angle     != right.angle )    ? left.angle    < right.angle :
                   ( left.x         != right.x )        ? left.x        < right.x :
                   ( left.y         != right.y )        ? left.y        < right.y :
                   false;
        }
    };
    
    using BroadPhaseLookup = std::multimap< FCSID, const FCSValue*, CompareFCSID >;
    
    std::vector< FCSValue > values;
    BroadPhaseLookup lookup;
    
    for( Site::PtrVector::const_iterator 
        i = m_area.getSpaces().begin(),
        iEnd = m_area.getSpaces().end(); i!=iEnd; ++i )
    {
        if( Area::Ptr pChildArea = boost::dynamic_pointer_cast< Area >( *i ) )
        {
            Feature_Contour::Ptr pContour = pChildArea->getContour();
            
            for( Feature_ContourSegment::Ptr pContourSegment : pChildArea->getBoundaries() )
            {
                Point2D ptContourBase;
                {
                    Feature_Contour::Ptr pContour = 
                        boost::dynamic_pointer_cast< Feature_Contour >( pContourSegment->Node::getParent() );
                    ptContourBase.x = pContour->getX( 0 );
                    ptContourBase.y = pContour->getY( 0 );
                }
                
                Point2D ptMid;
                {
                    ptMid.x = ptContourBase.x + pContourSegment->getX( Feature_ContourSegment::eMidPoint );
                    ptMid.y = ptContourBase.y + pContourSegment->getY( Feature_ContourSegment::eMidPoint );
                    pChildArea->getTransform().transform( ptMid.x, ptMid.y );
                }
                
                Math::Angle< 8 >::Value angle;
                {
                    Point2D ptEdge;
                    {
                        if( pChildArea->getTransform().isWindingInverted() )
                        {
                            ptEdge.x = ptContourBase.x + pContourSegment->getX( Feature_ContourSegment::eLeft );
                            ptEdge.y = ptContourBase.y + pContourSegment->getY( Feature_ContourSegment::eLeft );
                        }
                        else
                        {
                            ptEdge.x = ptContourBase.x + pContourSegment->getX( Feature_ContourSegment::eRight );
                            ptEdge.y = ptContourBase.y + pContourSegment->getY( Feature_ContourSegment::eRight );
                        }
                        pChildArea->getTransform().transform( ptEdge.x, ptEdge.y );
                    }
                    angle = Math::fromVector< Math::Angle< 8 > >( ptEdge.x - ptMid.x, ptEdge.y - ptMid.y );
                }
                    
                const FCSID id
                { 
                    static_cast< Math::Angle< 8 >::Value >( ( angle + 2 ) % 8 ),
                    static_cast< int >( Math::quantize( ptMid.x, fQuantisation ) ),
                    static_cast< int >( Math::quantize( ptMid.y, fQuantisation ) ) 
                };
                
                const FCSValue value{ id, pContourSegment.get(), ptMid.x, ptMid.y };
                
                values.push_back( value );
            }
        }
    }
    
    for( const FCSValue& value : values )
    {
        lookup.insert( std::make_pair( value.id, &value ) );
    }
    
    using FCSValuePtrSet = std::set< const FCSValue* >;
    FCSValuePtrSet openList;
    for( const FCSValue& value : values )
    {
        openList.insert( &value );
    }
    
    for( const FCSValue& value : values )
    {
        const FCSValue* pValue = &value;
        if( openList.find( pValue ) != openList.end() )
        {
            bool bFound = false;
            FCSID id = pValue->id;
            id.angle = Math::opposite< Math::Angle< 8 > >( id.angle );
            for( id.x = pValue->id.x - fQuantisation; ( id.x != pValue->id.x + fQuantisation ) && !bFound; id.x += fQuantisation )
            {
                for( id.y = pValue->id.y - fQuantisation; ( id.y != pValue->id.y + fQuantisation ) && !bFound; id.y += fQuantisation )
                {
                    BroadPhaseLookup::iterator i        = lookup.lower_bound( id );
                    BroadPhaseLookup::iterator iUpper   = lookup.upper_bound( id );
                    for( ; i!=iUpper && !bFound; ++i )
                    {
                        const FCSValue* pCompare = i->second;
                        if( openList.find( pCompare ) != openList.end() )
                        {
                            if( wykobi::distance( pValue->x, pValue->y, pCompare->x, pCompare->y ) < fConnectionMaxDist )
                            {
                                ConnectionPair cp{ pValue->pFCS, pCompare->pFCS };
                                
                                Connection::Ptr pConnection( new Connection( cp ) );
                                
                                auto r = m_connections.insert( std::make_pair( cp, pConnection ) );
                                VERIFY_RTE( r.second );
                                
                                openList.erase( pValue );
                                openList.erase( pCompare );
                                bFound = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

bool ConnectionAnalysis::isFeatureContourSegmentConnected( Feature_ContourSegment* pFeatureContourSegment ) const
{
    for( ConnectionPairMap::const_iterator i = m_connections.begin(),
        iEnd = m_connections.end(); i!=iEnd; ++i )
    {
        if( i->first.first == pFeatureContourSegment || 
            i->first.second == pFeatureContourSegment  )
        return true;    
    }
    return false;
}

}