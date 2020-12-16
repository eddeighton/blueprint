
#include "blueprint/visibility.h"
#include "blueprint/svgUtils.h"
#include "blueprint/object.h"
#include "blueprint/blueprint.h"

//#include "common/variant_utils.hpp"

namespace
{
    void renderFloorFace( Blueprint::Arr_with_hist_2& arr, Blueprint::Arr_with_hist_2::Face_const_handle hFace )
    {
        if( !hFace->is_unbounded() )
        {
            Blueprint::Arr_with_hist_2::Ccb_halfedge_const_circulator iter = hFace->outer_ccb();
            Blueprint::Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
            do
            {
                //test if the edge is a doorstep
                if( !iter->data().get() )
                {
                    CGAL::insert( arr,
                        Blueprint::Segment_2( iter->source()->point(),
                                              iter->target()->point() ) );
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
                //test if the edge is a doorstep
                if( !iter->data().get() )
                {
                    CGAL::insert( arr,
                        Blueprint::Segment_2( iter->source()->point(),
                                              iter->target()->point() ) );
                }
                ++iter;
            }
            while( iter != start );
        }
    }
}

namespace Blueprint
{
    
FloorAnalysis::FloorAnalysis( Compilation& compilation )
    :   m_hFloorFace( nullptr )
{
    Compilation::FaceHandleSet floorFaces;
    Compilation::FaceHandleSet fillerFaces;
    compilation.getFaces( floorFaces, fillerFaces );
    
    for( Compilation::FaceHandle hFace : floorFaces )
    {
        renderFloorFace( m_arr, hFace );
    }
    
    for( Site::Ptr pNestedSite : compilation.getBlueprint()->getSites() )
    {
        recurseObjects( pNestedSite );
    }
    
    {
        int iFloorFaceCount = 0;
        Arr_with_hist_2::Face_handle hOuterFace = m_arr.unbounded_face();
        for( Arr_with_hist_2::Hole_iterator
            holeIter = hOuterFace->holes_begin(),
            holeIterEnd = hOuterFace->holes_end();
                holeIter != holeIterEnd; ++holeIter )
        {
            Arr_with_hist_2::Ccb_halfedge_circulator iter = *holeIter;
            m_hFloorFace = iter->twin()->face();
            ++iFloorFaceCount;
        }
        VERIFY_RTE_MSG( iFloorFaceCount == 1, "Invalid number of floors: " << iFloorFaceCount );
    }
    
    VERIFY_RTE_MSG( !m_hFloorFace->is_unbounded(), "Floor face is unbounded" );
    
    
}

void FloorAnalysis::recurseObjects( Site::Ptr pSite )
{
    if( Object::Ptr pObject = boost::dynamic_pointer_cast< Object >( pSite ) )
    {
        boost::optional< Polygon2D > contourOpt = pObject->getContourPolygon();
        VERIFY_RTE( contourOpt );
        Compilation::renderContour( m_arr, pObject->getAbsoluteTransform(), 
            contourOpt.get(), wykobi::CounterClockwise );
    }
    //else if( Wall::Ptr pWall = boost::dynamic_pointer_cast< Wall >( pSite ) )
    //{
    //    
    //}
    
    for( Site::Ptr pNestedSite : pSite->getSites() )
    {
        recurseObjects( pNestedSite );
    }
}

void FloorAnalysis::render( const boost::filesystem::path& filepath )
{
    EdgeVectorVector edgeGroups;
    
    {
        EdgeVector edges;
        Arr_with_hist_2::Ccb_halfedge_const_circulator iter = m_hFloorFace->outer_ccb();
        Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
        do
        {
            edges.push_back( iter );
            ++iter;
        }
        while( iter != start );
        edgeGroups.push_back( edges );
    }
        
    {
        for( Arr_with_hist_2::Hole_const_iterator
            holeIter = m_hFloorFace->holes_begin(),
            holeIterEnd = m_hFloorFace->holes_end();
                holeIter != holeIterEnd; ++holeIter )
        {
            EdgeVector edges;
            Arr_with_hist_2::Ccb_halfedge_const_circulator iter = *holeIter;
            Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
            do
            {
                edges.push_back( iter );
                ++iter;
            }
            while( iter != start );
            if( !edges.empty() )
                edgeGroups.push_back( edges );
        }
    }

    generateHTML( filepath, m_arr, edgeGroups );
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace
{
    class ArrObserver : public CGAL::Arr_observer< Arr_with_hist_2 >
    {
    public:
        ArrObserver( Arr_with_hist_2& arr )
        :     CGAL::Arr_observer< Arr_with_hist_2 >( arr )
        {
          
        }

        virtual void before_split_face( Arr_with_hist_2::Face_handle, Arr_with_hist_2::Halfedge_handle e )
        {
        }

        virtual void before_merge_face( Arr_with_hist_2::Face_handle, Arr_with_hist_2::Face_handle, Arr_with_hist_2::Halfedge_handle e)
        {
        }
    };

    //Arr_with_hist_2::Point_2 min( const Arr_with_hist_2::Point_2& left, const Arr_with_hist_2::Point_2& right )
    //{
    //    return Arr_with_hist_2::Point_2( std::min( left.x(), right.x() ), std::min( left.y(), right.y() ) );
    //}
    //Arr_with_hist_2::Point_2 max( const Arr_with_hist_2::Point_2& left, const Arr_with_hist_2::Point_2& right )
    //{
    //    return Arr_with_hist_2::Point_2( std::max( left.x(), right.x() ), std::max( left.y(), right.y() ) );
    //}
    //        m_ptMin = min( m_ptMin, min( iter->source()->point(), iter->target()->point() ) );
    //        m_ptMax = max( m_ptMax, max( iter->source()->point(), iter->target()->point() ) );
    
    Segment_2 makeBisector( const Segment_2& edge, const Iso_rectangle_2& rect )
    {
        const Line_2 line( edge.source(), edge.target() );
        
        VERIFY_RTE_MSG( CGAL::do_intersect( rect, line ), 
            "bisector does not intersect bounds" );
        
        boost::optional< boost::variant< Point_2, Kernel::Segment_2 > > 
            result = CGAL::intersection< Kernel >( rect, line );
        VERIFY_RTE_MSG( result, "Bisector intersection failed" );
        
        return boost::get< Kernel::Segment_2 >( result.get() );
    }
}

Visibility::Visibility( FloorAnalysis& floor )
{
    Arr_with_hist_2::Face_const_handle hFloor = floor.getFloorFace();
    
    std::vector< Segment_2 > segments;
    
    {
        Arr_with_hist_2::Ccb_halfedge_const_circulator iter = hFloor->outer_ccb();
        Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
        do
        {
            segments.push_back( Segment_2(  iter->source()->point(),
                                            iter->target()->point() ) );
            ++iter;
        }
        while( iter != start );
    }
        
    {
        for( Arr_with_hist_2::Hole_const_iterator
            holeIter = hFloor->holes_begin(),
            holeIterEnd = hFloor->holes_end();
                holeIter != holeIterEnd; ++holeIter )
        {
            Arr_with_hist_2::Ccb_halfedge_const_circulator iter = *holeIter;
            Arr_with_hist_2::Ccb_halfedge_const_circulator start = iter;
            do
            {
                segments.push_back( Segment_2(  iter->source()->point(),
                                                iter->target()->point() ) );
                ++iter;
            }
            while( iter != start );
        }
    }
    
    //calculate the bounding box
    m_boundingBox = CGAL::bbox_2( segments.begin(), segments.end() );
    
    //for( const Segment_2& segment : segments )
    //{
    //    CGAL::insert( m_arr, segment );
    //}
    
    //ArrObserver observer( m_arr );
    
    //calculate the AABB
    
    for( const Segment_2& segment : segments )
    {
        Segment_2 bisector = makeBisector( segment, m_boundingBox );
        Arr_with_hist_2::Curve_handle hCurve = CGAL::insert( m_arr, bisector );
    }
    
}

void Visibility::render( const boost::filesystem::path& filepath )
{
    EdgeVectorVector edgeGroups;
    std::vector< Arr_with_hist_2::Halfedge_const_handle > edges;
    for( auto i = m_arr.edges_begin(); i != m_arr.edges_end(); ++i )
        edges.push_back( i );
    edgeGroups.push_back( edges );
    generateHTML( filepath, m_arr, edgeGroups );
}

}
