
#include "blueprint/compilation.h"
#include "blueprint/svgUtils.h"
#include "blueprint/blueprint.h"
#include "blueprint/connection.h"

#include <algorithm>

namespace
{
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

void Compilation::renderContour( Arr_with_hist_2& arr, const Matrix& transform, Polygon2D poly, int iOrientation )
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
        CGAL::insert( arr,
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
        renderContour( m_arr, transform, pSpace->getInteriorPolygon(), wykobi::CounterClockwise );

        //render the exterior polygons
        {
            for( const auto& p : pSpace->getInnerAreaExteriorPolygons() )
            {
                renderContour( m_arr, transform, p.second, wykobi::Clockwise );
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
        renderContour( m_arr, transform, pSpace->getContourPolygon().get(), wykobi::CounterClockwise );
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

void Compilation::getFaces( FaceHandleSet& floorFaces, FaceHandleSet& fillerFaces )
{    
    for( auto i = m_arr.faces_begin(),
        iEnd = m_arr.faces_end(); i!=iEnd; ++i )
    {
        Arr_with_hist_2::Face_const_handle hFace = i;
        //if( !hFace->is_unbounded() && !hFace->is_fictitious() )
        {
            if( doesFaceHaveDoorstep( hFace ) )
                floorFaces.insert( hFace );
            else 
            {
                //fillers cannot have holes
                if( hFace->holes_begin() == hFace->holes_end() )
                {
                    fillerFaces.insert( hFace );
                }
            }
        }
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
   

}