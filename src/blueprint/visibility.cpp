
#include "blueprint/visibility.h"
#include "blueprint/svgUtils.h"
#include "blueprint/object.h"
#include "blueprint/blueprint.h"

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
    
Visibility::Visibility( Compilation& compilation )
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
}

void Visibility::recurseObjects( Site::Ptr pSite )
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
