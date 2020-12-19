
#include "blueprint/space.h"
#include "blueprint/rasteriser.h"
#include "blueprint/cgalUtils.h"

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
    if( !m_pInteriorContourPathImpl.get() )
        m_pInteriorContourPathImpl.reset( new SimplePolygonMarkup( this, m_interiorPolygon, false ) );

    if( !( m_pContour = get< Feature_Contour >( "contour" ) ) )
    {
        m_pContour = Feature_Contour::Ptr( new Feature_Contour( getPtr(), "contour" ) );
        m_pContour->init();
        m_pContour->set( Utils::getDefaultPolygon() );
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
    
    setTranslation( m_transform, Map_FloorAverage()( x ), Map_FloorAverage()( y ) );
}

void Space::evaluate( const EvaluationMode& mode, EvaluationResults& results )
{
    const Kernel::FT wallWidth = 2;
                
    const Polygon& polygon = m_pContour->getPolygon();
    
    //calculate the site contour path and interior and exterior extrusions
    if( m_contourPolygon != polygon || m_interiorPolygon.is_empty() )
    {
        m_contourPolygon = polygon;
        
        m_exteriorPolygon.clear();
        m_interiorPolygon.clear();
        
        if( mode.bArrangement )
        {
            if( !m_contourPolygon.is_empty() && m_contourPolygon.is_simple() )
            {
                typedef boost::shared_ptr< Polygon > PolygonPtr ;
                typedef std::vector< PolygonPtr > PolygonPtrVector ;
                
                //calculate interior
                {
                    PolygonPtrVector inner_offset_polygons = 
                        CGAL::create_interior_skeleton_and_offset_polygons_2
                            < Kernel::FT, Polygon, Kernel, Kernel >
                            ( wallWidth, m_contourPolygon, ( Kernel() ), ( Kernel() ) );
                    if( !inner_offset_polygons.empty() )
                    {
                        m_interiorPolygon = *inner_offset_polygons.front();
                    }
                }
                
                //calculate exterior
                {
                    PolygonPtrVector outer_offset_polygons = 
                        CGAL::create_exterior_skeleton_and_offset_polygons_2
                            < Kernel::FT, Polygon, Kernel, Kernel >
                            ( wallWidth, m_contourPolygon, ( Kernel() ), ( Kernel() ) );
                    if( !outer_offset_polygons.empty() )
                    {
                        m_exteriorPolygon = *outer_offset_polygons.back();
                    }
                }
            }
        }
    }
    else if( !mode.bArrangement )
    {
        m_exteriorPolygon.clear();
        m_interiorPolygon.clear();
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
                Polygon poly = pSpace->getExteriorPolygon();
                if( !poly.is_empty() && poly.is_simple() )
                {
                    for( auto& p : poly )
                        p = pSpace->getTransform()( p );
                    
                    if( !poly.is_counterclockwise_oriented() )
                        poly.reverse_orientation();
                    
                    m_innerExteriors.push_back( poly );
                }
            }
        }
    }

    if( mode.bArrangement )
    {
        m_exteriorPolyMap.clear();
        
        //compute the union of ALL inner exterior contours
        std::vector< Polygon_with_holes > exteriorUnion;
        CGAL::join( m_innerExteriors.begin(), m_innerExteriors.end(), 
            std::back_inserter( exteriorUnion ) );
            
        int szCounter = 0;
        for( const Polygon_with_holes& polyWithHole : exteriorUnion )
        {
            if( !polyWithHole.is_unbounded() )
            {
                const Polygon& outer = polyWithHole.outer_boundary();
                if( !outer.is_empty() && outer.is_simple() )
                {
                    //clip the exterior to the interior
                    std::vector< Polygon_with_holes > clippedExterior;
                    CGAL::intersection( outer, m_interiorPolygon,
                        std::back_inserter( clippedExterior ) );
                        
                    //gather the outer boundaries of the results
                    for( const Polygon_with_holes& clip : clippedExterior )
                    {
                        if( !clip.is_unbounded() )
                        {
                            const Polygon& clipOuter = clip.outer_boundary();
                            if( !clipOuter.is_empty() && clipOuter.is_simple() )
                            {
                                m_exteriorPolyMap.insert( std::make_pair( szCounter++, clipOuter ) );
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        m_exteriorPolyMap.clear();
    }
}

}