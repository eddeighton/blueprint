
#include "blueprint/space.h"
#include "blueprint/rasteriser.h"

namespace Blueprint
{

const std::string& Space::TypeName()
{
    static const std::string strTypeName( "space" );
    return strTypeName;
}

Space::Space( Site::Ptr pParent, const std::string& strName )
    :   Site( pParent, strName )
{
    
}

Space::Space( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName )
{
    
}

Node::Ptr Space::copy( Node::Ptr pParent, const std::string& strName ) const
{
    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
    VERIFY_RTE( pSiteParent || !pParent );
    return Node::copy< Space >( 
        boost::dynamic_pointer_cast< const Space >( shared_from_this() ), pSiteParent, strName );
}

void Space::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );
    Site::save( node );
}
    
std::string Space::getStatement() const
{
    return Site::getStatement();
}

void Space::init()
{
    if( !m_pExteriorPolygons.get() )
        m_pExteriorPolygons.reset( new ExteriorGroupImpl( this, m_exteriorPolyMap, false ) );

    Site::init();
}

void Space::init( float x, float y )
{
    m_pContour = Feature_Contour::Ptr( new Feature_Contour( shared_from_this(), "contour" ) );
    m_pContour->init();
    add( m_pContour );
    
    Site::init();
    
    m_transform.setTranslation( Map_FloorAverage()( x ), Map_FloorAverage()( y ) );
}
    
void Space::evaluate( const EvaluationMode& mode, EvaluationResults& results )
{
    m_innerExteriors.clear();
    
    //bottom up recursion
    for( PtrVector::iterator i = m_sites.begin(),
        iEnd = m_sites.end(); i!=iEnd; ++i )
    {
        (*i)->evaluate( mode, results );
        
        if( Space::Ptr pSpace = boost::dynamic_pointer_cast< Space >( *i ) )
        {
            ClipperLib::Path inputClipperPath;
            Polygon2D exteriorPolygon = pSpace->getExteriorPolygon();
            for( Point2D& pt : exteriorPolygon )
            {
                pSpace->getTransform().transform( pt.x, pt.y );
                inputClipperPath.push_back( 
                    ClipperLib::IntPoint( 
                        static_cast< ClipperLib::cInt >( pt.x * CLIPPER_MAG ), 
                        static_cast< ClipperLib::cInt >( pt.y * CLIPPER_MAG ) ) ); 
            }
            if( wykobi::polygon_orientation( exteriorPolygon ) == wykobi::Clockwise )
                std::reverse( inputClipperPath.begin(), inputClipperPath.end() );
            m_innerExteriors.push_back( inputClipperPath );
        }
    }
    
    typedef PathImpl::AGGContainerAdaptor< Polygon2D > WykobiPolygonAdaptor;
    typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
    
    const Polygon2D& polygon = m_pContour->getPolygon();

    if( !m_polygonCache || 
        !( m_polygonCache.get().size() == polygon.size() ) || 
        !std::equal( polygon.begin(), polygon.end(), m_polygonCache.get().begin() ) || 
        !( m_innerExteriors.size() == m_innerExteriorsCache.size() ) ||
        !std::equal( m_innerExteriors.begin(), m_innerExteriors.end(), m_innerExteriorsCache.begin() ) 
        )
    {
        m_innerExteriorsCache = m_innerExteriors;
        m_polygonCache = polygon;
        
        PathImpl::aggPathToMarkupPath( 
            m_contourPath, 
            Adaptor( WykobiPolygonAdaptor( polygon ), true ) );
        
        static const double fExtrusionAmt = 2.0;
        
        ClipperLib::Path clipperPolygon;
        toClipperPoly( polygon, clipperPolygon );
        if( wykobi::polygon_orientation( polygon ) == wykobi::Clockwise )
            std::reverse( clipperPolygon.begin(), clipperPolygon.end() );
        
        ClipperLib::Paths interiorPaths, exteriorPath;
        extrudePoly( clipperPolygon, -fExtrusionAmt, interiorPaths );
        extrudePoly( clipperPolygon, fExtrusionAmt,  exteriorPath );
        
        fromClipperPolys( interiorPaths, m_interiorPolygon );
        fromClipperPolys( exteriorPath, m_exteriorPolygon );
        
        //calculate the exteriors
        {
            m_exteriorPolyMap.clear();
            int iCounter = 0;
            m_exteriorPolyMap.insert( std::make_pair( iCounter++, m_interiorPolygon ) );
            ClipperLib::Paths innerExteriorPolygons;
            if( unionAndClipPolygons( m_innerExteriors, interiorPaths, innerExteriorPolygons ) )
            {
                for( const ClipperLib::Path& exteriorPolygonPath : innerExteriorPolygons )
                {
                    Polygon2D exteriorPolygon;
                    fromClipperPoly( exteriorPolygonPath, exteriorPolygon );
                    m_exteriorPolyMap.insert( std::make_pair( iCounter++, exteriorPolygon ) );
                }
            }
            else
            {
                //TODO - report error
            }
        }
        
    }
}
}