
#include "blueprint/cgalSettings.h"
#include "blueprint/geometry.h"
#include "blueprint/blueprint.h"
#include "blueprint/space.h"
#include "blueprint/connection.h"
#include "blueprint/transform.h"

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

    using EdgeVector = std::vector< Blueprint::Arr_with_hist_2::Halfedge_const_handle >;
    using EdgeVectorVector = std::vector< EdgeVector >;

    void generateHTML( const boost::filesystem::path& filepath,
            const Blueprint::Arr_with_hist_2& arr,
            const EdgeVectorVector& edgeGroups )
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

        int iColour = 0;
        for( const EdgeVector& edges : edgeGroups )
        {
            const char* pszColour = SVG_COLOURS[ iColour ];
            iColour = ( iColour + 1 ) % SVG_COLOURS.size();
            for( Blueprint::Arr_with_hist_2::Halfedge_const_handle h : edges )
            {
                if( h->target()->point() == h->curve().source() )
                    h = h->twin();

                svgLine( h, minX, minY, scale, pszColour, *os );

                {
                    const double startX = ( to_double(  h->source()->point().x() ) - minX ) * scale;
                    const double startY = ( to_double( -h->source()->point().y() ) - minY ) * scale;
                    const double endX   = ( to_double(  h->target()->point().x() ) - minX ) * scale;
                    const double endY   = ( to_double( -h->target()->point().y() ) - minY ) * scale;

                    std::ostringstream osText;
                    {
                        //const void* pData       = h->data();
                        //const void* pTwinData   = h->twin()->data();
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
                                         "\" fill=\"green\" transform=\"rotate(30 " <<
                                            x << "," << y << ")\" >" << osText.str() << " </text>\n";
                    }
                }
            }
        }

        *os << "    </svg>\n";
        *os << "  </body>\n";
        *os << "</html>\n";

    }

    bool doesFaceHaveDoorstep( Blueprint::Arr_with_hist_2::Face_const_handle hFace )
    {
        if( !hFace->is_unbounded() )
        {
            Blueprint::Arr_with_hist_2::Ccb_halfedge_const_circulator iter = hFace->outer_ccb();
            Blueprint::Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
            do
            {
                if( iter->data().get() )
                {
                    return true;
                }
                ++iter;
            }
            while( iter != start );
        }

        //search through all holes
        for( Blueprint::Arr_with_hist_2::Hole_const_iterator
            holeIter = hFace->holes_begin(),
            holeIterEnd = hFace->holes_end();
                holeIter != holeIterEnd; ++holeIter )
        {
            Blueprint::Arr_with_hist_2::Ccb_halfedge_const_circulator iter = *holeIter;
            Blueprint::Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
            do
            {
                if( iter->data().get() )
                {
                    return true;
                }
                ++iter;
            }
            while( iter != start );
        }
        return false;
    }
            


}

namespace Blueprint
{

void Compilation::renderContour( const Matrix& transform, Polygon2D poly, int iOrientation )
{
    //transform to absolute coordinates
    for( Point2D& pt : poly )
        transform.transform( pt.x, pt.y );

    //ensure orientate counter clockwise
    const int polyOrientation = wykobi::polygon_orientation( poly );
    if( polyOrientation != iOrientation )
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

        //Curve_handle ch =
        CGAL::insert( m_arr,
            Segment_2(  Point_2( i->x,      i->y ),
                        Point_2( iNext->x,  iNext->y ) ) );
    }
}

void Compilation::recurse( Site::Ptr pSite )
{
    if( Space::Ptr pSpace = boost::dynamic_pointer_cast< Space >( pSite ) )
    {
        const Matrix transform = pSpace->getAbsoluteTransform();

        //render the interior polygon
        renderContour( transform, pSpace->getInteriorPolygon(), wykobi::CounterClockwise );

        //render the exterior polygons
        {
            for( const auto& p : pSpace->getInnerAreaExteriorPolygons() )
            {
                renderContour( transform, p.second, wykobi::Clockwise );
            }
        }
    }

    for( Site::Ptr pNestedSite : pSite->getSites() )
    {
        recurse( pNestedSite );
    }
}

void Compilation::recursePost( Site::Ptr pSite )
{
    if( Space::Ptr pSpace = boost::dynamic_pointer_cast< Space >( pSite ) )
    {
        const Matrix transform = pSpace->getAbsoluteTransform();

        //render the site polygon
        renderContour( transform, pSpace->getContourPolygon().get(), wykobi::CounterClockwise );
    }

    for( Site::Ptr pNestedSite : pSite->getSites() )
    {
        recursePost( pNestedSite );
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

    m_hDoorStep->set_data( (DefaultedBool( true )) );
    m_hDoorStep->twin()->set_data( (DefaultedBool( true )) );

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

        VERIFY_RTE_MSG( bFoundFirst && bFoundSecond, "Failed to construct connection: " << pConnection->Node::getName() );
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
    
    //record ALL doorsteps
    std::vector< Arr_with_hist_2::Halfedge_handle > edges;
    for( auto i = m_arr.edges_begin(); i != m_arr.edges_end(); ++i )
    {
        if( i->data().get() )
        {
            edges.push_back( i );
        }
    }
    
    for( Site::Ptr pSite : m_pBlueprint->getSites() )
    {
        recursePost( pSite );
    }
    
    for( Arr_with_hist_2::Halfedge_handle i : edges )
    {
        if( !i->data().get() )
        {
            THROW_RTE( "Doorstep edge lost after recursePost" );
        }
    }
}

Point2D getFaceInteriorPoint( Arr_with_hist_2::Face_const_handle hFace )
{
    Arr_with_hist_2::Ccb_halfedge_const_circulator halfedgeCirculator = hFace->outer_ccb();
    Arr_with_hist_2::Halfedge_const_handle hEdge = halfedgeCirculator;

    //deterine an interior point of the face
    const Point2D ptSource =
        wykobi::make_point< float >(
            to_double( hEdge->source()->point().x() ),
            to_double( hEdge->source()->point().y() ) );
    const Point2D ptTarget =
        wykobi::make_point< float >(
            to_double( hEdge->target()->point().x() ),
            to_double( hEdge->target()->point().y() ) );
    const Vector2D vDir = ptTarget - ptSource;
    const Vector2D vNorm =
        wykobi::normalize( 
            wykobi::make_vector< float >( -vDir.y, vDir.x ) );
    const Point2D ptMid = ptSource + vDir * 0.5f;
    const Point2D ptInterior = ptMid + vNorm * 0.1f;

    return ptInterior;
}

void faceToPolygon( Arr_with_hist_2::Face_const_handle hFace, Polygon2D& polygon )
{
    VERIFY_RTE( !hFace->is_unbounded() );

    Arr_with_hist_2::Ccb_halfedge_const_circulator iter = hFace->outer_ccb();
    Arr_with_hist_2::Ccb_halfedge_const_circulator first = iter;
    do
    {
        polygon.push_back(
            wykobi::make_point< float >(
                to_double( iter->source()->point().x() ),
                to_double( iter->source()->point().y() ) ) );
        ++iter;
    }
    while( iter != first );
}

void faceToPolygonWithHoles( Arr_with_hist_2::Face_const_handle hFace, PolygonWithHoles& polygon )
{
    if( !hFace->is_unbounded() )
    {
        Arr_with_hist_2::Ccb_halfedge_const_circulator iter = hFace->outer_ccb();
        Arr_with_hist_2::Ccb_halfedge_const_circulator first = iter;
        do
        {
            polygon.outer.push_back(
                wykobi::make_point< float >(
                    to_double( iter->source()->point().x() ),
                    to_double( iter->source()->point().y() ) ) );
            ++iter;
        }
        while( iter != first );
    }

    for( Arr_with_hist_2::Hole_const_iterator
        holeIter = hFace->holes_begin(),
        holeIterEnd = hFace->holes_end();
            holeIter != holeIterEnd; ++holeIter )
    {
        Polygon2D hole;
        Arr_with_hist_2::Ccb_halfedge_const_circulator iter = *holeIter;
        Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
        do
        {
            hole.push_back(
                wykobi::make_point< float >(
                    to_double( iter->source()->point().x() ),
                    to_double( iter->source()->point().y() ) ) );
            --iter;
        }
        while( iter != start );
        polygon.holes.emplace_back( hole );
    }
}

void wallSection(
        Arr_with_hist_2::Halfedge_const_handle hStart,
        Arr_with_hist_2::Halfedge_const_handle hEnd,
        Wall& wall )
{
    Arr_with_hist_2::Halfedge_const_handle hIter = hStart;
    do
    {
        if( hStart != hIter )
        {
            VERIFY_RTE_MSG( !hIter->data().get(), "Door step error" );
        }
        wall.points.push_back(
            wykobi::make_point< float >(
                to_double( hIter->target()->point().x() ),
                to_double( hIter->target()->point().y() ) ) );
        hIter = hIter->next();
    }
    while( hIter != hEnd );
}

void floorToWalls( Arr_with_hist_2::Face_const_handle hFloor, std::vector< Wall >& walls )
{
    using DoorStepVector = std::vector< Arr_with_hist_2::Halfedge_const_handle >;

    if( !hFloor->is_unbounded() )
    {
        //find the doorsteps if any
        DoorStepVector doorsteps;
        {
            //outer ccb winds COUNTERCLOCKWISE around the outer contour
            Arr_with_hist_2::Ccb_halfedge_const_circulator iter = hFloor->outer_ccb();
            Arr_with_hist_2::Ccb_halfedge_const_circulator first = iter;
            do
            {
                if( iter->data().get() )
                    doorsteps.push_back( iter );
                ++iter;
            }
            while( iter != first );
        }

        if( !doorsteps.empty() )
        {
            //iterate between each doorstep pair
            for( DoorStepVector::iterator i = doorsteps.begin(),
                iNext = doorsteps.begin(),
                iEnd = doorsteps.end(); i!=iEnd; ++i )
            {
                ++iNext;
                if( iNext == iEnd )
                    iNext = doorsteps.begin();

                Wall wall( false, true );
                wallSection( *i, *iNext, wall );
                walls.emplace_back( wall );
            }
        }
        else
        {
            Wall wall( true, true );
            Arr_with_hist_2::Ccb_halfedge_const_circulator iter = hFloor->outer_ccb();
            Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
            do
            {
                wall.points.push_back(
                    wykobi::make_point< float >(
                        to_double( iter->source()->point().x() ),
                        to_double( iter->source()->point().y() ) ) );
                ++iter;
            }
            while( iter != start );
            walls.emplace_back( wall );
        }
    }

    for( Arr_with_hist_2::Hole_const_iterator
        holeIter = hFloor->holes_begin(),
        holeIterEnd = hFloor->holes_end();
            holeIter != holeIterEnd; ++holeIter )
    {
        DoorStepVector doorsteps;
        {
            //the hole circulators wind CLOCKWISE around the hole contours
            Arr_with_hist_2::Ccb_halfedge_const_circulator iter = *holeIter;
            Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
            do
            {
                if( iter->data().get() )
                    doorsteps.push_back( iter );
                ++iter;
            }
            while( iter != start );
        }
        
        //iterate between each doorstep pair
        if( !doorsteps.empty() )
        {
            for( DoorStepVector::iterator i = doorsteps.begin(),
                iNext = doorsteps.begin(),
                iEnd = doorsteps.end(); i!=iEnd; ++i )
            {
                ++iNext;
                if( iNext == iEnd )
                    iNext = doorsteps.begin();

                Wall wall( false, false );
                wallSection( *i, *iNext, wall );
                walls.emplace_back( wall );
            }
        }
        else
        {
            Wall wall( true, false );
            Arr_with_hist_2::Ccb_halfedge_const_circulator iter = *holeIter;
            Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
            do
            {
                wall.points.push_back(
                    wykobi::make_point< float >(
                        to_double( iter->source()->point().x() ),
                        to_double( iter->source()->point().y() ) ) );
                ++iter;
            }
            while( iter != start );
            walls.emplace_back( wall );
        }
    }
}

void Compilation::findSpaceFaces( Space::Ptr pSpace, FaceHandleSet& faces, FaceHandleSet& spaceFaces )
{
    //check orientation
    Polygon2D spaceContourPolygon = pSpace->getContourPolygon().get();
    if( wykobi::polygon_orientation( spaceContourPolygon ) == wykobi::Clockwise )
        std::reverse( spaceContourPolygon.begin(), spaceContourPolygon.end() );
    const Matrix transform = pSpace->getAbsoluteTransform();
    for( Point2D& pt : spaceContourPolygon )
        transform.transform( pt.x, pt.y );
    
    std::vector< FaceHandleSet::iterator > removals;
    for( FaceHandleSet::iterator i = faces.begin(); i != faces.end(); ++i )
    {
        FaceHandle hFace = *i;
        if( !hFace->is_unbounded() )
        {
            //does the floor belong to the space?
            const Point2D ptInterior = getFaceInteriorPoint( hFace );
            if( wykobi::point_in_polygon( ptInterior, spaceContourPolygon ) )
            {
                spaceFaces.insert( hFace );
                removals.push_back( i );
            }
        }
    }
    
    for( std::vector< FaceHandleSet::iterator >::reverse_iterator 
            i = removals.rbegin(),
            iEnd = removals.rend(); i!=iEnd; ++i )
    {
        faces.erase( *i );
    }
}

void Compilation::recursePolyMap( Site::Ptr pSite, SpacePolyMap& spacePolyMap,
        FaceHandleSet& floorFaces, FaceHandleSet& fillerFaces )
{
    //bottom up recursion
    for( Site::Ptr pNestedSite : pSite->getSites() )
    {
        recursePolyMap( pNestedSite, spacePolyMap, floorFaces, fillerFaces );
    }

    if( Space::Ptr pSpace = boost::dynamic_pointer_cast< Space >( pSite ) )
    {
        FaceHandleSet spaceFloors;
        findSpaceFaces( pSpace, floorFaces, spaceFloors );

        FaceHandleSet spaceFillers;
        findSpaceFaces( pSpace, fillerFaces, spaceFillers );

        SpacePolyInfo::Ptr pSpacePolyInfo( new SpacePolyInfo );
        {
            for( FaceHandle hFloor : spaceFloors )
            {
                PolygonWithHoles polygon;
                faceToPolygonWithHoles( hFloor, polygon );
                pSpacePolyInfo->floors.emplace_back( polygon );
            }

            for( FaceHandle hFiller : spaceFillers )
            {
                Polygon2D polygon;
                faceToPolygon( hFiller, polygon );
                pSpacePolyInfo->fillers.emplace_back( polygon );
            }

            //determine walls
            for( FaceHandle hFloor : spaceFloors )
            {
                floorToWalls( hFloor, pSpacePolyInfo->walls );
            }

        }

        spacePolyMap.insert( std::make_pair( pSpace, pSpacePolyInfo ) );

    }

}

void Compilation::getSpacePolyMap( SpacePolyMap& spacePolyMap )
{
    //get all the floors and faces
    FaceHandleSet floorFaces, fillerFaces;
    for( auto i = m_arr.faces_begin(),
        iEnd = m_arr.faces_end(); i!=iEnd; ++i )
    {
        Arr_with_hist_2::Face_const_handle hFace = i;
        if( doesFaceHaveDoorstep( hFace ) )
            floorFaces.insert( hFace );
        else
            fillerFaces.insert( hFace );
    }


    for( Site::Ptr pSite : m_pBlueprint->getSites() )
    {
        recursePolyMap( pSite, spacePolyMap, floorFaces, fillerFaces );
    }

}

void Compilation::render( const boost::filesystem::path& filepath )
{
    EdgeVectorVector edgeGroups;
    std::vector< Arr_with_hist_2::Halfedge_const_handle > edges;
    for( auto i = m_arr.edges_begin(); i != m_arr.edges_end(); ++i )
        edges.push_back( i );
    edgeGroups.push_back( edges );
    generateHTML( filepath, m_arr, edgeGroups );
}


void Compilation::renderFloors( const boost::filesystem::path& filepath )
{
    EdgeVectorVector edgeGroups;

    using EdgeVector = std::vector< Arr_with_hist_2::Halfedge_const_handle >;
    for( auto i = m_arr.faces_begin(),
        iEnd = m_arr.faces_end(); i!=iEnd; ++i )
    {
        EdgeVector edges;
        //for each face determine if it has a doorstep - including only in the holes
        bool bDoesFaceHaveDoorstep = false;
        if( !i->is_unbounded() )
        {
            Arr_with_hist_2::Ccb_halfedge_const_circulator iter = i->outer_ccb();
            Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
            do
            {
                edges.push_back( iter );
                if( iter->data().get() )
                {
                    bDoesFaceHaveDoorstep = true;
                }
                ++iter;
            }
            while( iter != start );

            if( !bDoesFaceHaveDoorstep )
            {
                //search through all holes
                for( Arr_with_hist_2::Hole_const_iterator
                    holeIter = i->holes_begin(),
                    holeIterEnd = i->holes_end();
                        holeIter != holeIterEnd; ++holeIter )
                {
                    Arr_with_hist_2::Ccb_halfedge_const_circulator iter = *holeIter;
                    Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
                    do
                    {
                        if( iter->data().get() )
                        {
                            bDoesFaceHaveDoorstep = true;
                            break;
                        }
                        ++iter;
                    }
                    while( iter != start );
                }
            }
        }

        if( bDoesFaceHaveDoorstep )
        {
            for( Arr_with_hist_2::Hole_const_iterator
                holeIter = i->holes_begin(),
                holeIterEnd = i->holes_end();
                    holeIter != holeIterEnd; ++holeIter )
            {
                Arr_with_hist_2::Ccb_halfedge_const_circulator iter = *holeIter;
                Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
                do
                {
                    edges.push_back( iter );
                    ++iter;
                }
                while( iter != start );
            }
            edgeGroups.push_back( edges );
        }
    }

    generateHTML( filepath, m_arr, edgeGroups );
}


void Compilation::renderFillers( const boost::filesystem::path& filepath )
{
    EdgeVectorVector edgeGroups;

    for( auto i = m_arr.faces_begin(),
        iEnd = m_arr.faces_end(); i!=iEnd; ++i )
    {
        EdgeVector edges;
        //for each face determine if it has a doorstep - including only in the holes
        bool bDoesFaceHaveDoorstep = false;
        if( !i->is_unbounded() )
        {
            Arr_with_hist_2::Ccb_halfedge_const_circulator iter = i->outer_ccb();
            Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
            do
            {
                edges.push_back( iter );
                if( iter->data().get() )
                {
                    bDoesFaceHaveDoorstep = true;
                }
                ++iter;
            }
            while( iter != start );

            if( !bDoesFaceHaveDoorstep )
            {
                //search through all holes
                for( Arr_with_hist_2::Hole_const_iterator
                    holeIter = i->holes_begin(),
                    holeIterEnd = i->holes_end();
                        holeIter != holeIterEnd; ++holeIter )
                {
                    Arr_with_hist_2::Ccb_halfedge_const_circulator iter = *holeIter;
                    Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
                    do
                    {
                        if( iter->data().get() )
                        {
                            bDoesFaceHaveDoorstep = true;
                            break;
                        }
                        ++iter;
                    }
                    while( iter != start );
                }
            }
        }

        if( !bDoesFaceHaveDoorstep && !i->is_unbounded() )
        {
            for( Arr_with_hist_2::Hole_const_iterator
                holeIter = i->holes_begin(),
                holeIterEnd = i->holes_end();
                    holeIter != holeIterEnd; ++holeIter )
            {
                Arr_with_hist_2::Ccb_halfedge_const_circulator iter = *holeIter;
                Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
                do
                {
                    edges.push_back( iter );
                    ++iter;
                }
                while( iter != start );
            }
            edgeGroups.push_back( edges );
        }
    }

    generateHTML( filepath, m_arr, edgeGroups );
}
   
inline void loaddata( std::istream& is, Point2D& pt )
{
    is >> pt.x >> pt.y;
}
inline void loaddata( std::istream& is, Wall& data )
{
    is >> data.m_bClosed;
    is >> data.m_bCounterClockwise;
    std::size_t szSize;
    is >> szSize;
    for( std::size_t sz = 0; sz != szSize; ++sz )
    {
        Point2D pt;
        loaddata( is, pt );
        data.points.push_back( pt );
    }
}
inline void loaddata( std::istream& is, Polygon2D& data )
{
    std::size_t szSize;
    is >> szSize;
    for( std::size_t sz = 0; sz != szSize; ++sz )
    {
        Point2D pt;
        loaddata( is, pt );
        data.push_back( pt );
    }
}
inline void loaddata( std::istream& is, PolygonWithHoles& data )
{
    loaddata( is, data.outer );
    std::size_t szSize;
    is >> szSize;
    for( std::size_t sz = 0; sz != szSize; ++sz )
    {
        Polygon2D hole;
        loaddata( is, hole );
        data.holes.emplace_back( hole );
    }
}
inline void savedata( std::ostream& os, const Point2D& pt )
{
    os << pt.x << ' ' << pt.y << '\n';
}
         
inline void savedata( std::ostream& os, const Wall& data )
{
    os << data.m_bClosed << '\n';
    os << data.m_bCounterClockwise << '\n';
    os << data.points.size() << '\n';
    for( const Point2D& pt : data.points )
        savedata( os, pt );
}       
       
inline void savedata( std::ostream& os, const Polygon2D& data )
{
    os << data.size() << '\n';
    for( const Point2D& pt : data )
        savedata( os, pt );
}
            
inline void savedata( std::ostream& os, const PolygonWithHoles& data )
{
    savedata( os, data.outer );
    os << data.holes.size() << '\n';
    for( const Polygon2D& p : data.holes )
        savedata( os, p );
}
          
void Compilation::SpacePolyInfo::load( std::istream& is )
{
    {
        std::size_t szSize;
        is >> szSize;
        for( std::size_t sz = 0U; sz != szSize; ++sz )
        {
            PolygonWithHoles p;
            loaddata( is, p );
            floors.emplace_back( p );
        }
    }
    {
        std::size_t szSize;
        is >> szSize;
        for( std::size_t sz = 0U; sz != szSize; ++sz )
        {
            Polygon2D p;
            loaddata( is, p );
            fillers.emplace_back( p );
        }
    }
    {
        std::size_t szSize;
        is >> szSize;
        for( std::size_t sz = 0U; sz != szSize; ++sz )
        {
            Wall p( false, false );
            loaddata( is, p );
            walls.emplace_back( p );
        }
    }
}
 
void Compilation::SpacePolyInfo::save( std::ostream& os )
{
    os << floors.size() << '\n';
    for( const PolygonWithHoles& data : floors )
        savedata( os, data );
    os << fillers.size() << '\n';
    for( const Polygon2D& data : fillers )
        savedata( os, data );
    os << walls.size() << '\n';
    for( const Wall& data : walls )
        savedata( os, data );
}

}