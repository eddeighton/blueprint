#include "blueprint/basicarea.h"

//#include "blueprint/edit.h"

#include "ed/ed.hpp"

#include "common/rounding.hpp"

#include <cmath>

namespace Blueprint
{
    
void Area::ConnectionAnalysis::calculate()
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


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
const std::string& Area::TypeName()
{
    static const std::string strTypeName( "area" );
    return strTypeName;
}

Area::Area( Site::Ptr pParent, const std::string& strName )
    :   Site( pParent, strName ),
        m_connections( *this ),
        m_pSiteParent( pParent )
{
}

Area::Area( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName ),
        m_connections( *this ),
        m_pSiteParent( pParent ),
        m_transform( pOriginal->m_transform )
{
}

Area::~Area()
{
}

Node::Ptr Area::copy( Node::Ptr pParent, const std::string& strName ) const
{
    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
    VERIFY_RTE( pSiteParent || !pParent );
    return Node::copy< Area >( 
        boost::dynamic_pointer_cast< const Area >( shared_from_this() ), pSiteParent, strName );
}

void Area::init( float x, float y, bool bEmptyContour )
{
    if( bEmptyContour )
    {
        m_pContour = Feature_Contour::Ptr( new Feature_Contour( shared_from_this(), "contour" ) );
        m_pContour->init();
        add( m_pContour );
    }
    
    init();
    
    m_transform.setTranslation( Map_FloorAverage()( x ), Map_FloorAverage()( y ) );
}

void Area::init()
{
    Site::init();

    if( !( m_pContour = get< Feature_Contour >( "contour" ) ) )
    {
        m_pContour = Feature_Contour::Ptr( new Feature_Contour( shared_from_this(), "contour" ) );
        m_pContour->init();
        m_pContour->set( wykobi::make_rectangle< float >( -15, -15, 15, 15 ) );
        add( m_pContour );
    }
    
    if( !m_pPath.get() )
        m_pPath.reset( new PathImpl( m_path, this ) );
    
    m_boundaryPoints.clear();
    for_each_recursive( 
        generics::collectIfConvert( m_boundaryPoints, 
            Node::ConvertPtrType< Feature_ContourSegment >(), 
            Node::ConvertPtrType< Feature_ContourSegment >() ),
            Node::ConvertPtrType< Site >() );

    m_strLabelText.clear();
    m_properties.clear();
    {
        std::ostringstream os;
        os << Node::getName();
        {
            for_each_recursive( 
                generics::collectIfConvert( m_properties, 
                    Node::ConvertPtrType< Property >(), 
                    Node::ConvertPtrType< Property >() ),
                    Node::ConvertPtrType< Site >() );
                    
            for( Property::Ptr pProperty : m_properties )
            {
                os << "\n" << pProperty->getName() << ": " << pProperty->getValue();
            }
        }
        m_strLabelText = os.str();
    }

    if( !m_pLabel.get() )
        m_pLabel.reset( new TextImpl( this, m_strLabelText, 0.0f, 0.0f ) ); 
    
    if( !m_pPolygonGroup.get() )
        m_pPolygonGroup.reset( new MarkupGroupImpl( this, m_polygonMap ) ); 
    
    //if( !m_pBuffer ) m_pBuffer.reset( new NavBitmap( 1u, 1u ) );
}

void Area::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );
    
    if( boost::optional< const Ed::Shorthand& > shOpt = node.getShorty() )
    {
        Ed::IShorthandStream is( shOpt.get() );
        is >> m_transform;
    }
}

void Area::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Node::save( node );
    
    if( !node.statement.shorthand ) 
    {
        node.statement.shorthand = Ed::Shorthand();
    }
    
    {
        Ed::OShorthandStream os( node.statement.shorthand.get() );
        os << m_transform;
    }
}

std::string Area::getStatement() const
{
    std::ostringstream os;
    {
        Ed::Shorthand sh;
        {
            Ed::OShorthandStream ossh( sh );
            ossh << m_transform;
        }
        os << sh;
    }
    return os.str();
}

bool Area::canEvaluate( const Site::PtrVector& evaluated ) const
{
    bool bCanEvaluate = true;
    return bCanEvaluate;
}

Site::EvaluationResult Area::evaluate( const EvaluationMode& mode, DataBitmap& data )
{
    //bottom up recursion
    for( PtrVector::iterator i = m_spaces.begin(),
        iEnd = m_spaces.end(); i!=iEnd; ++i )
    {
        (*i)->evaluate( mode, data );
    }
    
    typedef PathImpl::AGGContainerAdaptor< wykobi::polygon< float, 2 > > WykobiPolygonAdaptor;
    typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
    
    if( m_pContour && m_pContour->isAutoCalculate() )
    {
        //calculate the interior contour...
        if( !m_spaces.empty() )
        {
            wykobi::rectangle< float > totalAABB;
            bool bFirst = true;
            for( Site::Ptr pChildSpace : m_spaces )
            {
                if( Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( pChildSpace ) )
                {
                    if( Feature_Contour::Ptr pContour = pArea->getContour() )
                    {
                        const wykobi::polygon< float, 2 >& polygon = pContour->getPolygon();
                        
                        wykobi::rectangle< float > areaAABB = wykobi::aabb( polygon );
                        pArea->getTransform().transform( areaAABB[ 0 ].x, areaAABB[ 0 ].y );
                        pArea->getTransform().transform( areaAABB[ 1 ].x, areaAABB[ 1 ].y );
                        
                        if( bFirst )
                        {
                            totalAABB = areaAABB;
                            bFirst = false;
                        }
                        else
                        {
                            totalAABB[ 0 ].x = std::min( areaAABB[ 0 ].x, totalAABB[0].x );
                            totalAABB[ 0 ].y = std::min( areaAABB[ 0 ].y, totalAABB[0].y );
                            totalAABB[ 1 ].x = std::max( areaAABB[ 1 ].x, totalAABB[1].x );
                            totalAABB[ 1 ].y = std::max( areaAABB[ 1 ].y, totalAABB[1].y );
                        }
                    }
                }
            }
            if( !bFirst )
            {
                wykobi::polygon< float, 2 > aabbPoly = wykobi::make_polygon( totalAABB );
                if( wykobi::polygon_orientation( aabbPoly ) == wykobi::Clockwise )
                    std::reverse( aabbPoly.begin(), aabbPoly.end() );
                m_pContour->set( aabbPoly );
            }
        }
    }

    const wykobi::polygon< float, 2u >& polygon = m_pContour->getPolygon();

    if( !m_polygonCache || 
        !( m_polygonCache.get().size() == polygon.size() ) || 
        !std::equal( polygon.begin(), polygon.end(), m_polygonCache.get().begin() ))
    {
        PathImpl::aggPathToMarkupPath( m_path, Adaptor( WykobiPolygonAdaptor( polygon ), true ) );
        m_polygonCache = polygon;
    }
    
    //calculate connections
    {
        m_connections.calculate();
        
        
        const ConnectionAnalysis::ConnectionPairMap& connections = m_connections.getConnections();
        
        //match the ConnectionAnalysis::ConnectionPairMap to the MarkupGroupImpl::PolyMap
        //both use the ConnectionAnalysis::ConnectionPair as a key
        
        struct Comparison
        {
            inline bool operator()( 
                    MarkupGroupImpl::PolyMap::iterator i,
                    ConnectionAnalysis::ConnectionPairMap::const_iterator j ) const
            {
                const ConnectionAnalysis::ConnectionPair& cp1 = i->first;
                const ConnectionAnalysis::ConnectionPair& cp2 = j->first;
                return  ( cp1.first  != cp2.first )  ? ( cp1.first  < cp2.first ) :
                        ( cp1.second != cp2.second ) ? ( cp1.second < cp2.second ) :
                        false;
                
            }
            inline bool opposite(
                    MarkupGroupImpl::PolyMap::iterator i,
                    ConnectionAnalysis::ConnectionPairMap::const_iterator j ) const
            {
                const ConnectionAnalysis::ConnectionPair& cp1 = i->first;
                const ConnectionAnalysis::ConnectionPair& cp2 = j->first;
                return  ( cp1.first  != cp2.first )  ? ( cp2.first  < cp1.first ) :
                        ( cp1.second != cp2.second ) ? ( cp2.second < cp1.second ) :
                        false;
            }
        };
        
        Comparison comparison;
        std::vector< MarkupGroupImpl::PolyMap::iterator > removals;
        std::vector< ConnectionAnalysis::ConnectionPairMap::const_iterator > additions;
        std::vector< std::pair< MarkupGroupImpl::PolyMap::iterator,
            ConnectionAnalysis::ConnectionPairMap::const_iterator > > updates;
        
        generics::matchGetUpdates( m_polygonMap.begin(), m_polygonMap.end(), connections.begin(), connections.end(),
                comparison,
                
                //should update
                [ &updates ]( MarkupGroupImpl::PolyMap::iterator i,
                    ConnectionAnalysis::ConnectionPairMap::const_iterator j)
                {
                    if( ( j->second->polygon.size() != i->second.size() ) || 
                        !std::equal( j->second->polygon.begin(), j->second->polygon.end(), i->second.begin() ) )
                    {
                        updates.push_back( std::make_pair( i, j ) );
                        return true;
                    }
                    return false;
                },
                
                //remove
                [ &removals ]( MarkupGroupImpl::PolyMap::iterator i )
                {
                    removals.push_back( i );
                },
                
                //add
                [ &additions ]( ConnectionAnalysis::ConnectionPairMap::const_iterator i )
                {
                    additions.push_back( i );
                },
                
                //update
                []( MarkupGroupImpl::PolyMap::const_iterator i )
                {
                    //do nothing.. handled by "should update" above
                }
            );
        
        for( MarkupGroupImpl::PolyMap::iterator i : removals )
        {
            m_polygonMap.erase( i );
        }    
        for( ConnectionAnalysis::ConnectionPairMap::const_iterator i : additions )
        {
            m_polygonMap[ i->first ] = i->second->polygon;
        }    
        for( const std::pair< MarkupGroupImpl::PolyMap::iterator,
                        ConnectionAnalysis::ConnectionPairMap::const_iterator >& iterPair : updates )
        {
            MarkupGroupImpl::PolyMap::iterator markupIter = iterPair.first;
            ConnectionAnalysis::ConnectionPairMap::const_iterator connectionIter = iterPair.second;
            
            markupIter->second = connectionIter->second->polygon;
        }    
                
    }
        
    Site::EvaluationResult result;
    return result;
}

void Area::setTransform( const Transform& transform )
{ 
    m_transform = transform; 
    setModified();
}
void Area::cmd_rotateLeft()
{
    m_transform.rotateLeft();
    setModified();
}

void Area::cmd_rotateRight()
{
    m_transform.rotateRight();
    setModified();
}

void Area::cmd_flipHorizontally()
{
    m_transform.flipHorizontally();
    setModified();
}

void Area::cmd_flipVertically()
{
    m_transform.flipVertically();
    setModified();
}

}