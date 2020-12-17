
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

    if( !( m_pContour = get< Feature_Contour >( "contour" ) ) )
    {
        m_pContour = Feature_Contour::Ptr( new Feature_Contour( getPtr(), "contour" ) );
        m_pContour->init();
        m_pContour->set( wykobi::make_rectangle< float >( -16, -16, 16, 16 ) );
        add( m_pContour );
    }
    
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
    const Polygon2D& polygon = m_pContour->getPolygon();
    
    //calculate the site contour path
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
    }

    //bottom up recursion
    m_innerExteriors.clear();
    for( PtrVector::iterator i = m_sites.begin(),
        iEnd = m_sites.end(); i!=iEnd; ++i )
    {
        (*i)->evaluate( mode, results );
        
        if( mode.bArrangement )
        {
            if( Space::Ptr pSpace = boost::dynamic_pointer_cast< Space >( *i ) )
            {
                ClipperLib::Path inputClipperPath;
                toClipperPoly( pSpace->getExteriorPolygon(), pSpace->getTransform(), 
                    wykobi::CounterClockwise, inputClipperPath );
                m_innerExteriors.push_back( inputClipperPath );
            }
        }
    }

    if( mode.bArrangement )
    {
        //m_exteriorPolygonCache is a cache of the site polygon to detect if need to recalculate the exterior polygon
        if( !(m_exteriorPolygonCache) || 
            !( m_exteriorPolygonCache.get().size() == polygon.size() ) || 
            !std::equal( polygon.begin(), polygon.end(), m_exteriorPolygonCache.get().begin() ) ||
            
            !( m_innerExteriors.size() == m_innerExteriorsCache.size() ) ||
            !std::equal( m_innerExteriors.begin(), m_innerExteriors.end(), m_innerExteriorsCache.begin() ) 
            )
        {
            m_exteriorPolygonCache = polygon;
            m_innerExteriorsCache = m_innerExteriors;
            
            static const double fExtrusionAmt = 2.0;
            
            ClipperLib::Path interiorPath;
            {
                ClipperLib::Path clipperPolygon;
                toClipperPoly( polygon, wykobi::CounterClockwise, clipperPolygon );
                if( wykobi::polygon_orientation( polygon ) == wykobi::Clockwise )
                    std::reverse( clipperPolygon.begin(), clipperPolygon.end() );
                
                ClipperLib::Paths interiorPaths, exteriorPath;
                extrudePoly( clipperPolygon, -fExtrusionAmt, interiorPaths );
                extrudePoly( clipperPolygon, fExtrusionAmt, exteriorPath );
                
                fromClipperPolys( interiorPaths, m_interiorPolygon );
                fromClipperPolys( exteriorPath, m_exteriorPolygon );
                
                if( !interiorPaths.empty() )
                {
                    interiorPath = interiorPaths.front();
                    //std::reverse( interiorPath.begin(), interiorPath.end() );
                }
            }
            
            //calculate the exteriors
            {
                m_exteriorPolyMap.clear();
                int iCounter = 0;
                m_exteriorPolyMap.insert( std::make_pair( iCounter++, m_interiorPolygon ) );
                ClipperLib::Paths innerExteriorPolygons;
                if( unionAndClipPolygons( m_innerExteriors, interiorPath, innerExteriorPolygons ) )
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
    else
    {
        m_exteriorPolygonCache.reset();
        m_exteriorPolyMap.clear();
        m_innerExteriorsCache.clear();
    }
}
}