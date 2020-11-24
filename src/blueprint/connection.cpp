

#include "blueprint/connection.h"
#include "blueprint/basicArea.h"

#include "common/assert_verify.hpp"

#include "wykobi_algorithm.hpp"

#include <algorithm>

namespace Blueprint
{

        
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
                float x = pContourSegment->getX( Feature_ContourSegment::eMidPoint );
                float y = pContourSegment->getY( Feature_ContourSegment::eMidPoint );
                
                const float xe = pContourSegment->getX( Feature_ContourSegment::eRight );
                const float ye = pContourSegment->getY( Feature_ContourSegment::eRight );
                
                const Math::Angle< 8 >::Value angle = 
                    Math::fromVector< Math::Angle< 8 > >( xe - x, ye - y );
                    
                //transform point relative to parent area
                {
                    Feature_Contour::Ptr pContour = 
                        boost::dynamic_pointer_cast< Feature_Contour >( pContourSegment->Node::getParent() );
                    x    += pContour->getX( 0 );
                    y    += pContour->getY( 0 );

                    pChildArea->getTransform().transform( x, y );
                }
                
                const FCSID id
                { 
                    static_cast< Math::Angle< 8 >::Value >( ( angle + 2 ) % 8 ),
                    static_cast< int >( Math::quantize( x, fQuantisation ) ),
                    static_cast< int >( Math::quantize( y, fQuantisation ) ) 
                };
                
                const FCSValue value{ id, pContourSegment.get(), x, y };
                
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
                                
                                wykobi::point2d< float > p1Left = wykobi::make_point< float >
                                (
                                    cp.first->getX( Feature_ContourSegment::eLeft ),
                                    cp.first->getY( Feature_ContourSegment::eLeft )
                                );
                                wykobi::point2d< float > p1Right = wykobi::make_point< float >
                                (
                                    cp.first->getX( Feature_ContourSegment::eRight ),
                                    cp.first->getY( Feature_ContourSegment::eRight )
                                );
                                
                                {
                                    Feature_Contour::Ptr pContour = boost::dynamic_pointer_cast< Feature_Contour >(
                                        cp.first->Node::getParent() );
                                    p1Left.x    += pContour->getX( 0 );
                                    p1Left.y    += pContour->getY( 0 );
                                    p1Right.x   += pContour->getX( 0 );
                                    p1Right.y   += pContour->getY( 0 );
                                    {
                                        Area::Ptr pChildOne = boost::dynamic_pointer_cast< Area >( 
                                            pContour->Node::getParent() );
                                        pChildOne->getTransform().transform( p1Left.x,  p1Left.y );
                                        pChildOne->getTransform().transform( p1Right.x, p1Right.y );
                                    }
                                }
                            
                                wykobi::point2d< float > p2Left = wykobi::make_point< float >
                                (
                                    cp.second->getX( Feature_ContourSegment::eLeft ),
                                    cp.second->getY( Feature_ContourSegment::eLeft )
                                );
                                wykobi::point2d< float > p2Right = wykobi::make_point< float >
                                (
                                    cp.second->getX( Feature_ContourSegment::eRight ),
                                    cp.second->getY( Feature_ContourSegment::eRight )
                                );
                                
                                {
                                    Feature_Contour::Ptr pContour = boost::dynamic_pointer_cast< Feature_Contour >(
                                        cp.second->Node::getParent() );
                                    p2Left.x    += pContour->getX( 0 );
                                    p2Left.y    += pContour->getY( 0 );
                                    p2Right.x   += pContour->getX( 0 );
                                    p2Right.y   += pContour->getY( 0 );
                                    {
                                        Area::Ptr pChildTwo = boost::dynamic_pointer_cast< Area >( 
                                            pContour->Node::getParent() );
                                        pChildTwo->getTransform().transform( p2Left.x,  p2Left.y );
                                        pChildTwo->getTransform().transform( p2Right.x, p2Right.y );
                                    }
                                }
                                
                                Connection::Ptr pConnection( new Connection );
                                {
                                    pConnection->polygon.push_back( p1Left );
                                    pConnection->polygon.push_back( p1Right );
                                    pConnection->polygon.push_back( p2Left );
                                    pConnection->polygon.push_back( p2Right );
                                }
                                
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