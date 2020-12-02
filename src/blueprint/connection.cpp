

#include "blueprint/connection.h"
#include "blueprint/rasteriser.h"

#include "common/assert_verify.hpp"
#include "common/rounding.hpp"

namespace Blueprint
{

const std::string& Connection::TypeName()
{
    static const std::string strTypeName( "connect" );
    return strTypeName;
}

Connection::Connection( Site::Ptr pParent, const std::string& strName )
    :   Site( pParent, strName )
{
    
}

Connection::Connection( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName )
{
    
}

Node::Ptr Connection::copy( Node::Ptr pParent, const std::string& strName ) const
{
    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
    VERIFY_RTE( pSiteParent || !pParent );
    return Node::copy< Connection >( 
        boost::dynamic_pointer_cast< const Connection >( shared_from_this() ), pSiteParent, strName );
}

void Connection::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );
    Site::save( node );
}
    
std::string Connection::getStatement() const
{
    return Site::getStatement();
}

void Connection::init()
{
    if( !( m_pControlPoint = get< Feature_Point >( "width" ) ) )
    {
        m_pControlPoint.reset( new Feature_Point( getPtr(), "width" ) );
        m_pControlPoint->init();
        m_pControlPoint->set( 0, 3.0f, 8.0f );
        add( m_pControlPoint );
    }
    
    Site::init();
}

void Connection::init( float x, float y )
{
    Site::init();
    
    m_transform.setTranslation( Map_FloorAverage()( x ), Map_FloorAverage()( y ) );
}
    
void Connection::evaluate( const EvaluationMode& mode, EvaluationResults& results )
{
    //bottom up recursion
    for( PtrVector::iterator i = m_sites.begin(),
        iEnd = m_sites.end(); i!=iEnd; ++i )
    {
        (*i)->evaluate( mode, results );
    }
    
    typedef PathImpl::AGGContainerAdaptor< Polygon2D > WykobiPolygonAdaptor;
    typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
    
    const float fWidth                  = fabsf( Math::quantize( m_pControlPoint->getX( 0 ), 1.0f ) );
    const float fConnectionHalfHeight   = fabsf( Math::quantize( m_pControlPoint->getY( 0 ), 1.0f ) );
    
    m_firstSegment = wykobi::make_segment( 
        -fWidth,    -fConnectionHalfHeight, 
        -fWidth,    fConnectionHalfHeight );
    m_secondSegment = wykobi::make_segment( 
        fWidth,     -fConnectionHalfHeight, 
        fWidth,     fConnectionHalfHeight );
    
    Polygon2D polygon;
    {
        polygon.push_back( m_firstSegment[ 0 ] );
        polygon.push_back( m_firstSegment[ 1 ] );
        polygon.push_back( wykobi::make_point< float >(  -fWidth / 2.0f, fConnectionHalfHeight ) );
        polygon.push_back( wykobi::make_point< float >(  0.0f,           fConnectionHalfHeight * 1.2f ) );
        polygon.push_back( wykobi::make_point< float >(  fWidth / 2.0f,  fConnectionHalfHeight ) );
        polygon.push_back( m_secondSegment[ 1 ] );
        polygon.push_back( m_secondSegment[ 0 ] );
    }
    
    if( !(m_polygonCache) || 
        !( m_polygonCache.get().size() == polygon.size() ) || 
        !std::equal( polygon.begin(), polygon.end(), m_polygonCache.get().begin() ) )
    {
        m_polygonCache = polygon;
        
        typedef PathImpl::AGGContainerAdaptor< Polygon2D > WykobiPolygonAdaptor;
        typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
        PathImpl::aggPathToMarkupPath( 
            m_contourPath, 
            Adaptor( WykobiPolygonAdaptor( polygon ), true ) );
    
    
    
        /*float fExtra = 2.0f;
        
        m_contourPath.clear();
        
        m_contourPath.push_back( MarkupPath::Cmd( m_firstSegment[ 0 ].x,  m_firstSegment[ 0 ].y - 1.0f,   MarkupPath::Cmd::path_cmd_move_to ) );
        m_contourPath.push_back( MarkupPath::Cmd( m_firstSegment[ 1 ].x,  m_firstSegment[ 1 ].y,   MarkupPath::Cmd::path_cmd_line_to ) );  
        
        m_contourPath.push_back( MarkupPath::Cmd( m_secondSegment[ 0 ].x, m_secondSegment[ 0 ].y, MarkupPath::Cmd::path_cmd_move_to ) );    
        m_contourPath.push_back( MarkupPath::Cmd( m_secondSegment[ 1 ].x, m_secondSegment[ 1 ].y, MarkupPath::Cmd::path_cmd_line_to ) );  

        m_contourPath.push_back( MarkupPath::Cmd( m_firstSegment[ 0 ].x,   m_firstSegment[ 0 ].y - fExtra + ( m_firstSegment[ 1 ].y - m_firstSegment[ 0 ].y ) / 2.0,   MarkupPath::Cmd::path_cmd_move_to ) );   
        m_contourPath.push_back( MarkupPath::Cmd( m_secondSegment[ 0 ].x,  m_firstSegment[ 0 ].y - fExtra + ( m_firstSegment[ 1 ].y - m_firstSegment[ 0 ].y ) / 2.0,   MarkupPath::Cmd::path_cmd_line_to ) );  

        m_contourPath.push_back( MarkupPath::Cmd( m_firstSegment[ 0 ].x,   m_firstSegment[ 0 ].y + fExtra + ( m_firstSegment[ 1 ].y - m_firstSegment[ 0 ].y ) / 2.0,   MarkupPath::Cmd::path_cmd_move_to ) );   
        m_contourPath.push_back( MarkupPath::Cmd( m_secondSegment[ 0 ].x,  m_firstSegment[ 0 ].y + fExtra + ( m_firstSegment[ 1 ].y - m_firstSegment[ 0 ].y ) / 2.0,   MarkupPath::Cmd::path_cmd_line_to ) );         
          */  
            
    }

}
  

}