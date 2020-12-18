

#include "blueprint/object.h"
#include "blueprint/rasteriser.h"

#include "common/assert_verify.hpp"

namespace Blueprint
{

const std::string& Object::TypeName()
{
    static const std::string strTypeName( "object" );
    return strTypeName;
}

Object::Object( Site::Ptr pParent, const std::string& strName )
    :   Site( pParent, strName )
{
    
}

Object::Object( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName )
{
    
}

Node::Ptr Object::copy( Node::Ptr pParent, const std::string& strName ) const
{
    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
    VERIFY_RTE( pSiteParent || !pParent );
    return Node::copy< Object >( 
        boost::dynamic_pointer_cast< const Object >( shared_from_this() ), pSiteParent, strName );
}

void Object::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );
    Site::save( node );
}
    
std::string Object::getStatement() const
{
    return Site::getStatement();
}

void Object::init()
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

void Object::init( Float x, Float y )
{
    //if( bEmptyContour )
    {
        m_pContour = Feature_Contour::Ptr( new Feature_Contour( shared_from_this(), "contour" ) );
        m_pContour->init();
        add( m_pContour );
    }
    
    Site::init();
    
    m_transform.setTranslation( Map_FloorAverage()( x ), Map_FloorAverage()( y ) );
}
    
void Object::evaluate( const EvaluationMode& mode, EvaluationResults& results )
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
        /*
        typedef PathImpl::AGGContainerAdaptor< Polygon2D > WykobiPolygonAdaptor;
        typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
        PathImpl::aggPathToMarkupPath( 
            m_contourPath, 
            Adaptor( WykobiPolygonAdaptor( polygon ), true ) );*/
    }

}
    

}