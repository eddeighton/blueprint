#include "mesh/osgMesh.h"
/*
#include <osg/Array>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Vec3>
#include <osg/ShapeDrawable>

#include "sgCore.h"

#include "common/assert_verify.hpp"

namespace Generator
{

osg::ref_ptr< osg::Geode > createBasicMesh( const PolyVox::SurfaceMesh< PolyVox::PositionMaterialNormal >& mesh )
{
    osg::ref_ptr< osg::Geode > pGeometryNode( new osg::Geode() );
    {
        osg::ref_ptr< osg::TriangleMesh > pMesh = new osg::TriangleMesh;

        osg::ref_ptr< osg::Vec3Array > pVertices( new osg::Vec3Array( mesh.getVertices().size() ) );

        osg::ref_ptr< osg::UIntArray > pIndicies( new osg::UIntArray( mesh.getIndices().begin(), mesh.getIndices().end() ) );

        osg::Vec3Array::iterator j = pVertices->begin();
        for( std::vector< PolyVox::PositionMaterialNormal >::const_iterator 
            i = mesh.getVertices().begin(),
            iEnd = mesh.getVertices().end(); i!=iEnd; ++i, ++j )
        {
            j->set( i->position.getX(), i->position.getY(), i->position.getZ() );
        }

        pMesh->setIndices( pIndicies.get() );
        pMesh->setVertices( pVertices.get() );
        
        osg::TessellationHints* pHints = new osg::TessellationHints;
        pHints->setDetailRatio( 2.0f );        
        osg::ref_ptr< osg::ShapeDrawable > pShape = new osg::ShapeDrawable( pMesh.get(), pHints );

        pGeometryNode->addDrawable( pShape.get() );
    }
    return pGeometryNode;
}

osg::ref_ptr< osg::Geode > polyVoxToOsgGeometry( const PolyVox::SurfaceMesh< PolyVox::PositionMaterialNormal >& mesh )
{
    osg::ref_ptr< osg::Geode > pGeometryNode( new osg::Geode );

    osg::ref_ptr< osg::Geometry > pGeometry( new osg::Geometry );
    {
        osg::ref_ptr< osg::Vec3Array > pVertices( new osg::Vec3Array( mesh.getVertices().size() ) );
        osg::ref_ptr< osg::Vec4Array > pColours( new osg::Vec4Array( mesh.getVertices().size() ) );
        osg::ref_ptr< osg::Vec3Array > pNormals( new osg::Vec3Array( mesh.getVertices().size() ) );
        osg::ref_ptr<osg::DrawElementsUInt> indices = 
            new osg::DrawElementsUInt( GL_TRIANGLES, mesh.getIndices().begin(), mesh.getIndices().end() );

        osg::Vec3Array::iterator j = pVertices->begin();
        osg::Vec3Array::iterator k = pNormals->begin();
        osg::Vec4Array::iterator l = pColours->begin();
        for( std::vector< PolyVox::PositionMaterialNormal >::const_iterator 
            i = mesh.getVertices().begin(),
            iEnd = mesh.getVertices().end(); i!=iEnd; ++i, ++j, ++k, ++l )
        {
            j->set( i->position.getX(), i->position.getY(), i->position.getZ() );
            k->set( i->normal.getX(), i->normal.getY(), i->normal.getZ() );
            l->set( 0.4f, 0.4f, 0.4f, 1.0f );
        }

        pGeometry->setVertexArray( pVertices.get() );
        pGeometry->addPrimitiveSet( indices.get() );
        pGeometry->setNormalArray( pNormals.get() );
        pGeometry->setColorArray( pColours.get() );

        pGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        pGeometry->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
    }

    pGeometryNode->addDrawable( pGeometry.get() );

    return pGeometryNode;
}


void addShape( osg::ref_ptr< osg::Geode > pGeometryNode, sgC3DObject* p3DObject )
{
    {
        p3DObject->Triangulate( SG_DELAUNAY_TRIANGULATION );
        //p3DObject->Triangulate( SG_VERTEX_TRIANGULATION );
        
        SG_MATERIAL mat;
        memset(&mat,0,sizeof(SG_MATERIAL));
        //mat.TextureScaleU = 1.0 / 16.0;
        //mat.TextureScaleV = 1.0 / 16.0;
        //mat.TextureSmooth = true;
       // mat.TextureMult = true;
        mat.TextureUVType = SG_CUBE_UV_TYPE;
        p3DObject->SetMaterial( mat );
        const SG_ALL_TRIANGLES* pTriangles = p3DObject->GetTriangles();
        ASSERT( pTriangles->allVertex );
        ASSERT( pTriangles->allUV );
    }

    osg::ref_ptr< osg::Geometry > pGeometry( new osg::Geometry );

    osg::ref_ptr< osg::Vec3Array > pVertices( new osg::Vec3Array );
    osg::ref_ptr< osg::Vec4Array > pColours( new osg::Vec4Array );
    osg::ref_ptr< osg::Vec3Array > pNormals( new osg::Vec3Array );
    osg::ref_ptr< osg::Vec2Array > pTexCoords( new osg::Vec2Array );
    osg::ref_ptr<osg::DrawElementsUInt> indices = 
        new osg::DrawElementsUInt( GL_TRIANGLES );
                            
    const SG_ALL_TRIANGLES* pTriangles = p3DObject->GetTriangles();
    for( int i = 0; i < pTriangles->nTr * 3; ++i )
    {
        pVertices->push_back( 
            osg::Vec3( 
                pTriangles->allVertex[ i ].x, 
                pTriangles->allVertex[ i ].y, 
                pTriangles->allVertex[ i ].z ) );
        pNormals->push_back( 
            osg::Vec3( 
                pTriangles->allNormals[ i ].x, 
                pTriangles->allNormals[ i ].y, 
                pTriangles->allNormals[ i ].z ) );
        pColours->push_back( 
            osg::Vec4( 1,1,1, 1.0f ) );
        pTexCoords->push_back( 
            osg::Vec2( 
                pTriangles->allUV[ i * 2 ], 
                pTriangles->allUV[ i * 2 + 1 ] ) );
        indices->push_back( i );
    }
                            
    pGeometry->setVertexArray( pVertices.get() );
    pGeometry->addPrimitiveSet( indices.get() );
    pGeometry->setNormalArray( pNormals.get() );
    pGeometry->setColorArray( pColours.get() );
    pGeometry->setTexCoordArray( 0, pTexCoords.get() );

    pGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    pGeometry->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );

    pGeometryNode->addDrawable( pGeometry.get() );
}

void addList( osg::ref_ptr< osg::Geode > pGeometryNode, IObjectsList* pObjects )
{
    for( sgCObject* pObject = pObjects->GetHead();
        pObject; pObject = pObjects->GetNext( pObject ) )
    {
        switch( pObject->GetType() )
        {
            case SG_OT_BAD_OBJECT :
	        case SG_OT_POINT      :
	        case SG_OT_LINE       :
	        case SG_OT_CIRCLE     :
	        case SG_OT_ARC        :
	        case SG_OT_SPLINE     :
	        case SG_OT_TEXT       :
	        case SG_OT_CONTOUR    :
	        case SG_OT_DIM        :
                break;
	        case SG_OT_3D         : 
                addShape( pGeometryNode, (sgC3DObject*)pObject );
                break;
	        case SG_OT_GROUP      : 
                addList( pGeometryNode, ((sgCGroup*)pObject)->GetChildrenList() );
                break;
        }
    }
}

osg::ref_ptr< osg::Geode > createQGCoreMesh( boost::shared_ptr< sgCGroup > pGroup )
{
    osg::ref_ptr< osg::Geode > pGeometryNode( new osg::Geode );
    
    if( pGroup )
        addList( pGeometryNode, pGroup->GetChildrenList() );

    return pGeometryNode;
}

}*/
