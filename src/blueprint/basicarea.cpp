#include "blueprint/basicarea.h"
#include "blueprint/spaceUtils.h"

#include "ed/ed.hpp"

#include "common/rounding.hpp"

#include <cmath>

namespace Blueprint
{

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
        m_exteriors( *this, m_connections ),
        m_pSiteParent( pParent )
{
}

Area::Area( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName ),
        m_connections( *this ),
        m_exteriors( *this, m_connections ),
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
    
    if( !m_pConnectionPolygons.get() )
        m_pConnectionPolygons.reset( new ConnectionGroupImpl( this, m_connectionPolyMap, true ) );
    if( !m_pExteriorPolygons.get() )
        m_pExteriorPolygons.reset( new ExteriorGroupImpl( this, m_exteriorPolyMap, false ) );
    
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
    
    typedef PathImpl::AGGContainerAdaptor< Polygon2D > WykobiPolygonAdaptor;
    typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
    
    if( m_pContour && m_pContour->isAutoCalculate() )
    {
        //calculate the interior contour...
        if( !m_spaces.empty() )
        {
            Rect2D totalAABB;
            bool bFirst = true;
            for( Site::Ptr pChildSpace : m_spaces )
            {
                if( Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( pChildSpace ) )
                {
                    if( Feature_Contour::Ptr pContour = pArea->getContour() )
                    {
                        Polygon2D polygon = pContour->getPolygon();
                        for( Point2D& pt : polygon )
                            pArea->getTransform().transform( pt.x, pt.y );
                        const Rect2D areaAABB = wykobi::aabb( polygon );
                        
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
                Polygon2D aabbPoly = wykobi::make_polygon( totalAABB );
                if( wykobi::polygon_orientation( aabbPoly ) == wykobi::Clockwise )
                    std::reverse( aabbPoly.begin(), aabbPoly.end() );
                m_pContour->set( aabbPoly );
            }
        }
    }

    const Polygon2D& polygon = m_pContour->getPolygon();

    if( !m_polygonCache || 
        !( m_polygonCache.get().size() == polygon.size() ) || 
        !std::equal( polygon.begin(), polygon.end(), m_polygonCache.get().begin() ))
    {
        PathImpl::aggPathToMarkupPath( m_path, Adaptor( WykobiPolygonAdaptor( polygon ), true ) );
        m_polygonCache = polygon;
    }
    
    //connections
    {
        m_connections.calculate();
        
        const ConnectionAnalysis::ConnectionPairMap& connections = m_connections.getConnections();
        
        //match the ConnectionAnalysis::ConnectionPairMap to the ConnectionGroupImpl::PolyMap
        //both use the ConnectionAnalysis::ConnectionPair as a key
        
        struct Comparison
        {
            inline bool operator()( 
                    ConnectionGroupImpl::PolyMap::iterator i,
                    ConnectionAnalysis::ConnectionPairMap::const_iterator j ) const
            {
                const ConnectionAnalysis::ConnectionPair& cp1 = i->first;
                const ConnectionAnalysis::ConnectionPair& cp2 = j->first;
                return  ( cp1.first  != cp2.first )  ? ( cp1.first  < cp2.first ) :
                        ( cp1.second != cp2.second ) ? ( cp1.second < cp2.second ) :
                        false;
                
            }
            inline bool opposite(
                    ConnectionGroupImpl::PolyMap::iterator i,
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
        std::vector< ConnectionGroupImpl::PolyMap::iterator > removals;
        std::vector< ConnectionAnalysis::ConnectionPairMap::const_iterator > additions;
        std::vector< std::pair< ConnectionGroupImpl::PolyMap::iterator,
            ConnectionAnalysis::ConnectionPairMap::const_iterator > > updates;
        
        generics::matchGetUpdates( 
                m_connectionPolyMap.begin(), m_connectionPolyMap.end(), 
                connections.begin(), connections.end(),
                comparison,
                
                //should update
                [ &updates ]( ConnectionGroupImpl::PolyMap::iterator i,
                    ConnectionAnalysis::ConnectionPairMap::const_iterator j)
                {
                    if( ( j->second->getPolygon().size() != i->second.size() ) || 
                        !std::equal( j->second->getPolygon().begin(), j->second->getPolygon().end(), i->second.begin() ) )
                    {
                        updates.push_back( std::make_pair( i, j ) );
                        return true;
                    }
                    return false;
                },
                
                //remove
                [ &removals ]( ConnectionGroupImpl::PolyMap::iterator i )
                {
                    removals.push_back( i );
                },
                
                //add
                [ &additions ]( ConnectionAnalysis::ConnectionPairMap::const_iterator i )
                {
                    additions.push_back( i );
                },
                
                //update
                []( ConnectionGroupImpl::PolyMap::const_iterator i )
                {
                    //do nothing.. handled by "should update" above
                }
            );
        
        for( ConnectionGroupImpl::PolyMap::iterator i : removals )
        {
            m_connectionPolyMap.erase( i );
        }    
        for( ConnectionAnalysis::ConnectionPairMap::const_iterator i : additions )
        {
            m_connectionPolyMap[ i->first ] = i->second->getPolygon();
        }    
        for( const std::pair< ConnectionGroupImpl::PolyMap::iterator,
                        ConnectionAnalysis::ConnectionPairMap::const_iterator >& iterPair : updates )
        {
            ConnectionGroupImpl::PolyMap::iterator markupIter = iterPair.first;
            ConnectionAnalysis::ConnectionPairMap::const_iterator connectionIter = iterPair.second;
            markupIter->second = connectionIter->second->getPolygon();
        }
    }
    
    //exteriors
    {
        m_exteriors.calculate();
        
        m_exteriorPolyMap.clear();
        const ExteriorAnalysis::Exterior::PtrVector& exteriors = m_exteriors.getExteriors();
        int iCounter = 0;
        for( const ExteriorAnalysis::Exterior::Ptr pExterior : exteriors )
        {
            m_exteriorPolyMap.insert( std::make_pair( iCounter++, pExterior->getPolygon() ) );
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