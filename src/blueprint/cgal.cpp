
#include "blueprint/cgalSettings.h"
#include "blueprint/geometry.h"
#include "blueprint/blueprint.h"
#include "blueprint/space.h"
#include "blueprint/connection.h"

#include "common/file.hpp"

namespace 
{
    
static const std::vector< const char* > SVG_COLOURS =
{
    "blue",
    "green",
    "red",
    "yellow",
    "orange",
    "purple",
    "brown",
    "black"                   
};

inline double to_double( const CGAL::Quotient< CGAL::MP_Float >& q )
{
    return 
        CGAL::INTERN_MP_FLOAT::to_double( q.numerator() ) /
        CGAL::INTERN_MP_FLOAT::to_double( q.denominator() );
}

void svgLine( Blueprint::Arr_with_hist_2::Halfedge_const_handle h, double minX, double minY, double scale, const char* pszColour, std::ostream& os )
{
    if( h->target()->point() == h->curve().source() )
        h = h->twin();
    
    const double startX = ( to_double(  h->source()->point().x() ) - minX ) * scale;
    const double startY = ( to_double( -h->source()->point().y() ) - minY ) * scale;
    const double endX   = ( to_double(  h->target()->point().x() ) - minX ) * scale;
    const double endY   = ( to_double( -h->target()->point().y() ) - minY ) * scale;
    
    std::ostringstream osEdge;
    osEdge << 
        startX << "," << startY << " " << 
        ( startX + ( endX - startX ) / 2.0 ) << "," << ( startY + ( endY - startY ) / 2.0 ) << " " << 
        endX << "," << endY;
    
    os << "       <polyline points=\"" << osEdge.str() << "\" style=\"fill:none;stroke:" << pszColour << ";stroke-width:1\" marker-mid=\"url(#mid)\" />\n";
    os << "       <circle cx=\"" << startX << "\" cy=\"" << startY << "\" r=\"3\" stroke=\"" << pszColour << "\" stroke-width=\"1\" fill=\"" << pszColour << "\" />\n";
    os << "       <circle cx=\"" << endX << "\" cy=\"" << endY << "\" r=\"3\" stroke=\"" << pszColour << "\" stroke-width=\"1\" fill=\"" << pszColour << "\" />\n";

}

void generateHTML( const boost::filesystem::path& filepath, const Blueprint::Arr_with_hist_2& arr )
{
    std::unique_ptr< boost::filesystem::ofstream > os =
        createNewFileStream( filepath );
        
    double scale = 10.0;
    double  minX = std::numeric_limits< double >::max(), 
            minY = std::numeric_limits< double >::max();
    double  maxX = -std::numeric_limits< double >::max(), 
            maxY = -std::numeric_limits< double >::max();
    for( auto i = arr.edges_begin(); i != arr.edges_end(); ++i )
    {
        {
            const double x = to_double( i->source()->point().x() );
            const double y = to_double( -i->source()->point().y() );
            if( x < minX ) minX = x;
            if( y < minY ) minY = y;
            if( x > maxX ) maxX = x;
            if( y > maxY ) maxY = y;
        }
        
        {
            const double x = to_double( i->target()->point().x() );
            const double y = to_double( -i->target()->point().y() );
            if( x < minX ) minX = x;
            if( y < minY ) minY = y;
            if( x > maxX ) maxX = x;
            if( y > maxY ) maxY = y;
        }
    }
    const double sizeX = maxX - minX;
    const double sizeY = maxY - minY;
        
    *os << "<!DOCTYPE html>\n";
    *os << "<html>\n";
    *os << "  <head>\n";
    *os << "    <title>Compilation Output</title>\n";
    *os << "  </head>\n";
    *os << "  <body>\n";
    *os << "    <h1>" << filepath.string() << "</h1>\n";
    
    *os << "    <svg width=\"" << 100 + sizeX * scale << "\" height=\"" << 100 + sizeY * scale << "\" >\n";
    *os << "      <defs>\n";
    *os << "      <marker id=\"mid\" markerWidth=\"10\" markerHeight=\"10\" refX=\"0\" refY=\"3\" orient=\"auto\" markerUnits=\"strokeWidth\">\n";
    *os << "      <path d=\"M0,0 L0,6 L9,3 z\" fill=\"#f00\" />\n";
    *os << "      </marker>\n";
    *os << "      </defs>\n";
    *os << "       <text x=\"" << 10 << "\" y=\"" << 10 << 
                     "\" fill=\"green\"  >Vertex Count: " << arr.number_of_vertices() << " </text>\n";
    *os << "       <text x=\"" << 10 << "\" y=\"" << 20 << 
                     "\" fill=\"green\"  >Edge Count: " << arr.number_of_edges() << " </text>\n";
                     
    for( auto i = arr.edges_begin(); i != arr.edges_end(); ++i )
    {
        Blueprint::Arr_with_hist_2::Halfedge_const_handle h = i;
        
        if( i->target()->point() == i->curve().source() )
            h = h->twin();
        
        svgLine( h, minX, minY, scale, "blue", *os );
        
        {
            const double startX = ( to_double(  h->source()->point().x() ) - minX ) * scale;
            const double startY = ( to_double( -h->source()->point().y() ) - minY ) * scale;
            const double endX   = ( to_double(  h->target()->point().x() ) - minX ) * scale;
            const double endY   = ( to_double( -h->target()->point().y() ) - minY ) * scale;
            
            std::ostringstream osText;
            {
                const void* pData       = h->data();
                const void* pTwinData   = h->twin()->data();
                /*if( const Blueprint::Area* pArea = (const Blueprint::Area*)pData )
                {
                    osText << "l:" << pArea->getName();
                }
                else
                {
                    osText << "l:";
                }
                if( const Blueprint::Area* pArea = (const Blueprint::Area*)pTwinData )
                {
                    osText << " r:" << pArea->getName();
                }
                else
                {
                    osText << " r:";
                }*/
            }
            {
                float x = startX + ( endX - startX ) / 2.0f;
                float y = startY + ( endY - startY ) / 2.0f;
                *os << "       <text x=\"" << x << "\" y=\"" << y << 
                                 "\" fill=\"green\" transform=\"rotate(30 " << x << "," << y << ")\" >" << osText.str() << " </text>\n";
            }
        }
    }
    
    *os << "    </svg>\n";
    *os << "  </body>\n";
    *os << "</html>\n";

}
}


namespace Blueprint
{
    
void Compilation::recurse( Site::Ptr pSite )
{
    
    if( Space::Ptr pSpace = boost::dynamic_pointer_cast< Space >( pSite ) )
    {
        const Matrix transform = pSpace->getAbsoluteTransform();
        
        //render the interior polygon
        {
            Polygon2D poly = pSpace->getInteriorPolygon();
            
            //transform to absolute coordinates
            for( Point2D& pt : poly )
                transform.transform( pt.x, pt.y );
            
            //ensure orientate counter clockwise
            const int polyOrientation = wykobi::polygon_orientation( poly );
            if( polyOrientation != wykobi::CounterClockwise )
            {
                std::reverse( poly.begin(), poly.end() );
            }
            
            //render the line segments
            for( auto i = poly.begin(),
                iNext = poly.begin(),
                iEnd = poly.end();
                i!=iEnd; ++i )
            {
                ++iNext;
                if( iNext == iEnd ) iNext = poly.begin();
                
                Curve_handle ch = CGAL::insert( m_arr, 
                    Segment_2(  Point_2( i->x,      i->y ),
                                Point_2( iNext->x,  iNext->y ) ) );
            }
        }
        
        //render the exterior polygons
        {
            for( const auto& p : pSpace->getInnerAreaExteriorPolygons() )
            {
                Polygon2D poly = p.second;
                
                //transform to absolute coordinates
                for( Point2D& pt : poly )
                    transform.transform( pt.x, pt.y );
                
                //ensure orientate clockwise
                const int polyOrientation = wykobi::polygon_orientation( poly );
                if( polyOrientation != wykobi::Clockwise )
                {
                    std::reverse( poly.begin(), poly.end() );
                }
                
                //render the line segments
                for( auto i = poly.begin(),
                    iNext = poly.begin(),
                    iEnd = poly.end();
                    i!=iEnd; ++i )
                {
                    ++iNext;
                    if( iNext == iEnd ) iNext = poly.begin();
                    
                    Curve_handle ch = CGAL::insert( m_arr, 
                        Segment_2(  Point_2( i->x,      i->y ),
                                    Point_2( iNext->x,  iNext->y ) ) );
                }
            }
        }
    }
    
    for( Site::Ptr pNestedSite : pSite->getSites() )
    {
        recurse( pNestedSite );
    }
}


void constructConnectionEdges( Arr_with_hist_2& arr, Connection::Ptr pConnection, 
        Arr_with_hist_2::Halfedge_handle firstBisectorEdge, 
        Arr_with_hist_2::Halfedge_handle secondBisectorEdge )
{
    
    Arr_with_hist_2::Vertex_handle vFirstStart  = firstBisectorEdge->source();
    Arr_with_hist_2::Vertex_handle vFirstEnd    = firstBisectorEdge->target();
    Arr_with_hist_2::Vertex_handle vSecondStart = secondBisectorEdge->source();
    Arr_with_hist_2::Vertex_handle vSecondEnd   = secondBisectorEdge->target();
    
    const Point_2 ptFirstStart  = vFirstStart->point();
    const Point_2 ptFirstEnd    = vFirstEnd->point();
    const Point_2 ptSecondStart = vSecondStart->point();
    const Point_2 ptSecondEnd   = vSecondEnd->point();
    const Point_2 ptFirstMid    = ptFirstStart  + ( ptFirstEnd - ptFirstStart )   / 2.0;
    const Point_2 ptSecondMid   = ptSecondStart + ( ptSecondEnd - ptSecondStart ) / 2.0;
        
    Arr_with_hist_2::Halfedge_handle hFirstStartToMid = 
        arr.split_edge( firstBisectorEdge, ptFirstMid );
    Arr_with_hist_2::Vertex_handle vFirstMid = hFirstStartToMid->target();
    
    Arr_with_hist_2::Halfedge_handle hSecondStartToMid = 
        arr.split_edge( secondBisectorEdge, ptSecondMid );
    Arr_with_hist_2::Vertex_handle vSecondMid = hSecondStartToMid->target();
    
    //create edge between mid points
    Arr_with_hist_2::Halfedge_handle m_hDoorStep;
    {
        const Segment_2 segDoorStep( ptFirstMid, ptSecondMid );
        m_hDoorStep = arr.insert_at_vertices( segDoorStep, vFirstMid, vSecondMid );
    }
    
    //m_hDoorStep->set_data( m_pFirstStart->getArea() );
    //m_hDoorStep->twin()->set_data( m_pSecondStart->getArea() );
    
    {
        bool bFound = false;
        Arr_with_hist_2::Halfedge_around_vertex_circulator first, iter;
        first = iter = vFirstStart->incident_halfedges();
        do
        {
            if( ( iter->source() == vSecondStart ) || ( iter->source() == vSecondEnd ) )
            {
                arr.remove_edge( iter );
                bFound = true;
                break;
            }
            ++iter;
        }
        while( iter != first );
        VERIFY_RTE( bFound );
    }
    {
        bool bFound = false;
        Arr_with_hist_2::Halfedge_around_vertex_circulator first, iter;
        first = iter = vFirstEnd->incident_halfedges();
        do
        {
            if( ( iter->source() == vSecondStart ) || ( iter->source() == vSecondEnd ) )
            {
                arr.remove_edge( iter );
                bFound = true;
                break;
            }
            ++iter;
        }
        while( iter != first );
        VERIFY_RTE( bFound );
    }
}

void Compilation::connect( Site::Ptr pSite )
{
    if( Connection::Ptr pConnection = boost::dynamic_pointer_cast< Connection >( pSite ) )
    {
        const Matrix transform = pConnection->getAbsoluteTransform();
    
        //attempt to find the four connection vertices
        std::vector< Arr_with_hist_2::Halfedge_handle > toRemove;
        Arr_with_hist_2::Halfedge_handle firstBisectorEdge, secondBisectorEdge;
        bool bFoundFirst = false, bFoundSecond = false;
        {
            Segment2D firstSeg  = pConnection->getFirstSegment();
            transform.transform( firstSeg[ 0 ].x, firstSeg[ 0 ].y );
            transform.transform( firstSeg[ 1 ].x, firstSeg[ 1 ].y );
            
            const Point_2 ptFirstStart( firstSeg[ 0 ].x, firstSeg[ 0 ].y );
            const Point_2 ptFirstEnd( firstSeg[ 1 ].x, firstSeg[ 1 ].y );
            
            Curve_handle firstCurve = CGAL::insert( m_arr, 
                Segment_2( ptFirstStart, ptFirstEnd ) );
            
            for( auto   i = m_arr.induced_edges_begin( firstCurve );
                        i != m_arr.induced_edges_end( firstCurve ); ++i )
            {
                Arr_with_hist_2::Halfedge_handle h = *i;
                
                if( ( h->source()->point() == ptFirstStart ) || 
                    ( h->source()->point() == ptFirstEnd   ) || 
                    ( h->target()->point() == ptFirstStart ) || 
                    ( h->target()->point() == ptFirstEnd   ) )
                {
                    toRemove.push_back( h );
                }
                else 
                {
                    firstBisectorEdge = h;
                    VERIFY_RTE( !bFoundFirst );
                    bFoundFirst = true;
                }
            }
        }
        
        {
            Segment2D secondSeg = pConnection->getSecondSegment();
            transform.transform( secondSeg[ 0 ].x, secondSeg[ 0 ].y );
            transform.transform( secondSeg[ 1 ].x, secondSeg[ 1 ].y );
            
            const Point_2 ptSecondStart( secondSeg[ 1 ].x, secondSeg[ 1 ].y );
            const Point_2 ptSecondEnd( secondSeg[ 0 ].x, secondSeg[ 0 ].y );
            
            Curve_handle secondCurve = CGAL::insert( m_arr, 
                Segment_2( ptSecondStart, ptSecondEnd ) );
                
            for( auto   i = m_arr.induced_edges_begin( secondCurve );
                        i != m_arr.induced_edges_end( secondCurve ); ++i )
            {
                Arr_with_hist_2::Halfedge_handle h = *i;
                
                if( ( h->source()->point() == ptSecondStart ) || 
                    ( h->source()->point() == ptSecondEnd   ) || 
                    ( h->target()->point() == ptSecondStart ) || 
                    ( h->target()->point() == ptSecondEnd   ) )
                {
                    toRemove.push_back( h );
                }
                else 
                {
                    secondBisectorEdge = h;
                    VERIFY_RTE( !bFoundSecond );
                    bFoundSecond = true;
                }
            }
        }
        
        VERIFY_RTE_MSG( bFoundFirst && bFoundSecond, "Failed to construct connection" );
        constructConnectionEdges( m_arr, pConnection, firstBisectorEdge, secondBisectorEdge );
        
        //VERIFY_RTE_MSG( toRemove.size() == 4, "Bad connection" );
        for( Arr_with_hist_2::Halfedge_handle h : toRemove )
        {
            m_arr.remove_edge( h );
        }
        
    }
    
    for( Site::Ptr pNestedSite : pSite->getSites() )
    {
        connect( pNestedSite );
    }
    
}
    
Compilation::Compilation( Blueprint::Ptr pBlueprint )
    :   m_pBlueprint( pBlueprint )
{
    for( Site::Ptr pSite : m_pBlueprint->getSites() )
    {
        recurse( pSite );
    }
    for( Site::Ptr pSite : m_pBlueprint->getSites() )
    {
        connect( pSite );
    }
    
    for( Arr_with_hist_2::Halfedge_handle h : m_arr.halfedge_handles() )
    {
        h->set_data( nullptr );
        h->twin()->set_data( nullptr );
    }
    
    
    
}
    
void Compilation::render( const boost::filesystem::path& filepath )
{
    generateHTML( filepath, m_arr );
}


}