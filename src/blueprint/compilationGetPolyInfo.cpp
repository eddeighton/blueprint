
#include "blueprint/cgalSettings.h"
#include "blueprint/geometry.h"
#include "blueprint/blueprint.h"
#include "blueprint/space.h"
#include "blueprint/connection.h"
#include "blueprint/transform.h"
#include "blueprint/compilation.h"

namespace
{
    /*
    inline Blueprint::Point2D make_source_point( Blueprint::Arrangement::Halfedge_const_handle halfedge )
    {
        return wykobi::make_point< float >(
                CGAL::to_double( halfedge->source()->point().x() ),
                CGAL::to_double( halfedge->source()->point().y() ) );
    }
    inline Blueprint::Point2D make_target_point( Blueprint::Arrangement::Halfedge_const_handle halfedge )
    {
        return wykobi::make_point< float >(
                CGAL::to_double( halfedge->target()->point().x() ),
                CGAL::to_double( halfedge->target()->point().y() ) );
    }
    
        
    Blueprint::Point2D getFaceInteriorPoint( Blueprint::Arrangement::Face_const_handle hFace )
    {
        Blueprint::Arrangement::Ccb_halfedge_const_circulator halfedgeCirculator = hFace->outer_ccb();
        Blueprint::Arrangement::Halfedge_const_handle hEdge = halfedgeCirculator;

        //deterine an interior point of the face
        const Blueprint::Point2D ptSource = make_source_point( hEdge );
        const Blueprint::Point2D ptTarget = make_target_point( hEdge );
        
        const Blueprint::Vector2D vDir = ptTarget - ptSource;
        const Blueprint::Vector2D vNorm =
            wykobi::normalize( 
                wykobi::make_vector< float >( -vDir.y, vDir.x ) );
        const Blueprint::Point2D ptMid = ptSource + vDir * 0.5f;
        const Blueprint::Point2D ptInterior = ptMid + vNorm * 0.1f;

        return ptInterior;
    }  */
    /*
    void faceToPolygon( Blueprint::Arrangement::Face_const_handle hFace, Blueprint::Polygon2D& polygon )
    {
        VERIFY_RTE( !hFace->is_unbounded() );

        Blueprint::Arrangement::Ccb_halfedge_const_circulator iter = hFace->outer_ccb();
        Blueprint::Arrangement::Ccb_halfedge_const_circulator first = iter;
        do
        {
            polygon.push_back( make_source_point( iter ) );
            ++iter;
        }
        while( iter != first );
    }

    void faceToPolygonWithHoles( Blueprint::Arrangement::Face_const_handle hFace, Blueprint::PolygonWithHoles& polygon )
    {
        if( !hFace->is_unbounded() )
        {
            Blueprint::Arrangement::Ccb_halfedge_const_circulator iter = hFace->outer_ccb();
            Blueprint::Arrangement::Ccb_halfedge_const_circulator first = iter;
            do
            {
                polygon.outer.push_back( make_source_point( iter ) );
                ++iter;
            }
            while( iter != first );
        }

        for( Blueprint::Arrangement::Hole_const_iterator
            holeIter = hFace->holes_begin(),
            holeIterEnd = hFace->holes_end();
                holeIter != holeIterEnd; ++holeIter )
        {
            Blueprint::Polygon2D hole;
            Blueprint::Arrangement::Ccb_halfedge_const_circulator iter = *holeIter;
            Blueprint::Arrangement::Ccb_halfedge_const_circulator start = iter;
            do
            {
                hole.push_back( make_source_point( iter ) );
                --iter;
            }
            while( iter != start );
            polygon.holes.emplace_back( hole );
        }
    }*/

    void wallSection(
            Blueprint::Arrangement::Halfedge_const_handle hStart,
            Blueprint::Arrangement::Halfedge_const_handle hEnd,
            Blueprint::Wall& wall )
    {
        THROW_RTE( "TODO" );
        /*
        Blueprint::Arrangement::Halfedge_const_handle hIter = hStart;
        do
        {
            if( hStart != hIter )
            {
                VERIFY_RTE_MSG( !hIter->data().get(), "Door step error" );
            }
            wall.points.push_back( make_target_point( hIter ) );
            hIter = hIter->next();
        }
        while( hIter != hEnd );*/
    }

    void floorToWalls( Blueprint::Arrangement::Face_const_handle hFloor, std::vector< Blueprint::Wall >& walls )
    {
        THROW_RTE( "TODO" );
        /*
        using DoorStepVector = std::vector< Blueprint::Arrangement::Halfedge_const_handle >;

        if( !hFloor->is_unbounded() )
        {
            //find the doorsteps if any
            DoorStepVector doorsteps;
            {
                //outer ccb winds COUNTERCLOCKWISE around the outer contour
                Blueprint::Arrangement::Ccb_halfedge_const_circulator iter = hFloor->outer_ccb();
                Blueprint::Arrangement::Ccb_halfedge_const_circulator first = iter;
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

                    Blueprint::Wall wall( false, true );
                    wallSection( *i, *iNext, wall );
                    walls.emplace_back( wall );
                }
            }
            else
            {
                Blueprint::Wall wall( true, true );
                Blueprint::Arrangement::Ccb_halfedge_const_circulator iter = hFloor->outer_ccb();
                Blueprint::Arrangement::Ccb_halfedge_const_circulator start = iter;
                do
                {
                    wall.points.push_back( make_source_point( iter ) );
                    ++iter;
                }
                while( iter != start );
                walls.emplace_back( wall );
            }
        }

        for( Blueprint::Arrangement::Hole_const_iterator
            holeIter = hFloor->holes_begin(),
            holeIterEnd = hFloor->holes_end();
                holeIter != holeIterEnd; ++holeIter )
        {
            DoorStepVector doorsteps;
            {
                //the hole circulators wind CLOCKWISE around the hole contours
                Blueprint::Arrangement::Ccb_halfedge_const_circulator iter = *holeIter;
                Blueprint::Arrangement::Ccb_halfedge_const_circulator start = iter;
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

                    Blueprint::Wall wall( false, false );
                    wallSection( *i, *iNext, wall );
                    walls.emplace_back( wall );
                }
            }
            else
            {
                Blueprint::Wall wall( true, false );
                Blueprint::Arrangement::Ccb_halfedge_const_circulator iter = *holeIter;
                Blueprint::Arrangement::Ccb_halfedge_const_circulator start = iter;
                do
                {
                    wall.points.push_back( make_source_point( iter ) );
                    ++iter;
                }
                while( iter != start );
                walls.emplace_back( wall );
            }
        }*/
    }

}

namespace Blueprint
{

void Compilation::findSpaceFaces( Space::Ptr pSpace, FaceHandleSet& faces, FaceHandleSet& spaceFaces )
{
    
    THROW_RTE( "TODO" );
    /*
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
    }*/
}

void Compilation::recursePolyMap( Site::Ptr pSite, SpacePolyMap& spacePolyMap,
        FaceHandleSet& floorFaces, FaceHandleSet& fillerFaces )
{
    
    THROW_RTE( "TODO" );
    /*
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

    }*/

}

void Compilation::getSpacePolyMap( SpacePolyMap& spacePolyMap )
{
    //get all the floors and faces
    FaceHandleSet floorFaces, fillerFaces;
    getFaces( floorFaces, fillerFaces );

    for( Site::Ptr pSite : m_pBlueprint->getSites() )
    {
        recursePolyMap( pSite, spacePolyMap, floorFaces, fillerFaces );
    }

}

}