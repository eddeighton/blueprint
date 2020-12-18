
#include "blueprint/wall.h"
#include "blueprint/rasteriser.h"

namespace Blueprint
{

const std::string& Wall::TypeName()
{
    static const std::string strTypeName( "wall" );
    return strTypeName;
}

Wall::Wall( Site::Ptr pParent, const std::string& strName )
    :   Site( pParent, strName )
{
    
}

Wall::Wall( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName )
{
    
}

Node::Ptr Wall::copy( Node::Ptr pParent, const std::string& strName ) const
{
    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
    VERIFY_RTE( pSiteParent || !pParent );
    return Node::copy< Wall >( 
        boost::dynamic_pointer_cast< const Wall >( shared_from_this() ), pSiteParent, strName );
}

void Wall::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );
    Site::save( node );
}
    
std::string Wall::getStatement() const
{
    return Site::getStatement();
}

void Wall::init()
{
    THROW_RTE( "TODO" );
    /*
    if( !( m_pContour = get< Feature_Contour >( "contour" ) ) )
    {
        m_pContour = Feature_Contour::Ptr( new Feature_Contour( getPtr(), "contour" ) );
        m_pContour->init();
        m_pContour->set( wykobi::make_rectangle< Float >( -16, -16, 16, 16 ) );
        add( m_pContour );
    }
    
    Site::init();*/
}

void Wall::init( Float x, Float y )
{
    m_pContour = Feature_Contour::Ptr( new Feature_Contour( shared_from_this(), "contour" ) );
    m_pContour->init();
    add( m_pContour );
    
    Site::init();
    
    m_transform.setTranslation( Map_FloorAverage()( x ), Map_FloorAverage()( y ) );
}
    
    
void Wall::evaluate( const EvaluationMode& mode, EvaluationResults& results )
{
    //bottom up recursion
    for( PtrVector::iterator i = m_sites.begin(),
        iEnd = m_sites.end(); i!=iEnd; ++i )
    {
        (*i)->evaluate( mode, results );
    }
    
    
    typedef PathImpl::AGGContainerAdaptor< Polygon2D > WykobiPolygonAdaptor;
    typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
    
    const Polygon& polygon = m_pContour->getPolygon();
    
    if( !m_polygonCache || 
        !( m_polygonCache.get().size() == polygon.size() ) || 
        !std::equal( polygon.begin(), polygon.end(), m_polygonCache.get().begin() ) )
    {
        m_polygonCache = polygon;
        
        /*typedef PathImpl::AGGContainerAdaptor< Polygon2D > WykobiPolygonAdaptor;
        typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
        PathImpl::aggPathToMarkupPath( 
            m_contourPath, 
            Adaptor( WykobiPolygonAdaptor( polygon ), true ) );*/
    }

}
  
}