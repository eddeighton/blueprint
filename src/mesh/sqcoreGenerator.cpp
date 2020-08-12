#include "mesh/sqcoreGenerator.h"
/*
#include <set>

#include "common\assert_verify.hpp"
#include "common\compose.hpp"

#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>

#include "sgCore.h"

#include "blueprint\databitmap.h"

namespace Generator
{
typedef std::vector< sgC3DObject* > Object3DVector;
typedef std::vector< sgCObject* > ObjectVector;

void deleteSCObject( sgCObject* pObject )
{
    if( pObject->GetType() == SG_OT_GROUP )
    {
        sgCGroup* pGroup = (sgCGroup*)pObject;
        ObjectVector groupObjects( pGroup->GetChildrenList()->GetCount() );
        if( pGroup->BreakGroup( groupObjects.data() ) )
        {
            std::for_each( groupObjects.begin(), groupObjects.end(),
                boost::bind( &deleteSCObject, _1 ) );
        }
    }
    else if( pObject->GetType() == SG_OT_CONTOUR )
    {
        sgCContour* pContour = (sgCContour*)pObject;
        ObjectVector groupObjects( pContour->GetChildrenList()->GetCount() );
        if( pContour->BreakContour( groupObjects.data() ) )
        {
            std::for_each( groupObjects.begin(), groupObjects.end(),
                boost::bind( &deleteSCObject, _1 ) );
        }
    }
    sgDeleteObject( pObject );
}

template< class T >
boost::shared_ptr< T > makeSharedGSPtr( T* pObject )
{
    return boost::shared_ptr< T >( pObject, boost::bind( &deleteSCObject, _1 ) );
}

typedef boost::shared_ptr< sgCGroup > sgCGroupPtr;
typedef boost::shared_ptr< sgC3DObject > sgC3DObjectPtr;

struct Result
{
    Object3DVector subtractions, additions;
};

void collectObjects( sgCGroupPtr pGroup, Object3DVector& objects )
{
    if( pGroup )
    {
        ObjectVector groupObjects( pGroup->GetChildrenList()->GetCount() );
        const bool bBreakResult = pGroup->BreakGroup( groupObjects.data() );
        ASSERT( bBreakResult );
        if( bBreakResult )
        {
            for( ObjectVector::iterator 
                i = groupObjects.begin(),
                iEnd = groupObjects.end(); i!=iEnd; ++i )
            {
                sgCObject* pObject = *i;
                switch( pObject->GetType() )
                {
                    default:
                    case SG_OT_BAD_OBJECT :
	                case SG_OT_POINT      :
	                case SG_OT_LINE       :
	                case SG_OT_CIRCLE     :
	                case SG_OT_ARC        :
	                case SG_OT_SPLINE     :
	                case SG_OT_TEXT       :
	                case SG_OT_CONTOUR    :
	                case SG_OT_DIM        :
                        ASSERT( false );
                        sgDeleteObject( pObject );
                        break;
	                case SG_OT_3D         : 
                        objects.push_back( (sgC3DObject*)pObject );
                        break;
	                case SG_OT_GROUP      : 
                        collectObjects( makeSharedGSPtr< sgCGroup >( (sgCGroup*)pObject ), objects );
                        break;
                }
            }
        }
    }
}

boost::shared_ptr< sgCContour > constructContour( const Blueprint::Site::FloatPairVector& points, 
                                                 double dHeight, double dScale, double fMinX, double fMinY, double fInset )
{
    std::vector< sgCObject* > lineSegments;
    for( Blueprint::Site::FloatPairVector::const_iterator 
        i = points.begin(), 
        iNext = points.begin(), 
        iEnd = points.end();
        i!=iEnd; ++i )
    {
        ++iNext;
        if( iNext == iEnd ) 
            iNext = points.begin();
        lineSegments.push_back( sgCreateLine(   i->first * dScale - fMinX,       i->second * dScale - fMinY,  dHeight * dScale, 
                                            iNext->first * dScale - fMinX,   iNext->second * dScale - fMinY,  dHeight * dScale ) );
    }
    boost::shared_ptr< sgCContour > pContour =
        makeSharedGSPtr< sgCContour >( sgCContour::CreateContour( lineSegments.data(), lineSegments.size() ) );
    
    ASSERT( pContour );
    ASSERT( !pContour->IsSelfIntersecting() );
    ASSERT( pContour->IsClosed() );

    SG_POINT planeNormal = { 0.0,0.0,1.0};
    if( pContour->GetOrient( planeNormal ) != sgC2DObject::OO_ANTICLOCKWISE )
        pContour->ChangeOrient();
    ASSERT( pContour->GetOrient( planeNormal ) == sgC2DObject::OO_ANTICLOCKWISE );
    
    boost::shared_ptr< sgCContour > pContourInset = 
        makeSharedGSPtr< sgCContour >( pContour->GetEquidistantContour( fInset * dScale, fInset * dScale, false ) );
    ASSERT( pContourInset );
    if( pContourInset->GetOrient( planeNormal ) != sgC2DObject::OO_ANTICLOCKWISE )
        pContourInset->ChangeOrient();
    ASSERT( pContourInset->GetOrient( planeNormal ) == sgC2DObject::OO_ANTICLOCKWISE );
    ASSERT( !pContourInset->IsSelfIntersecting() );
    ASSERT( pContourInset->IsClosed() );
    return pContourInset;
}
    
void collectObjects( Result& result, Blueprint::Node::Ptr pNode, double dHeight, double dScale, double fMinX, double fMinY )
{
    if( Blueprint::Site::Ptr pSite = 
        boost::dynamic_pointer_cast< Blueprint::Site >( pNode ) )
    {
        if( Blueprint::Site::Ptr pSiteParent = 
            boost::dynamic_pointer_cast< Blueprint::Site >( pNode->getParent() ) )
        {
            if( Blueprint::Site::Ptr pSiteParentParent = 
                boost::dynamic_pointer_cast< Blueprint::Site >( pSiteParent->Node::getParent() ) )
            {
                //sub space
                Blueprint::Site::FloatPairVector contour;
                pSite->getContour( contour );

                boost::shared_ptr< sgCContour > pContour1 = constructContour( contour, 0.0,            dScale, fMinX, fMinY, 0.1 );
                boost::shared_ptr< sgCContour > pContour2 = constructContour( contour, dHeight / 2.0,  dScale, fMinX, fMinY, 1.0 );

                sgC3DObjectPtr pSurface = makeSharedGSPtr< sgC3DObject >(
                    (sgC3DObject*)sgSurfaces::LinearSurfaceFromSections( *pContour1, *pContour2, 0.0, false ) );
                
                sgC3DObjectPtr pLowerFace = makeSharedGSPtr< sgC3DObject >(
                    (sgC3DObject*)sgSurfaces::Face( *pContour1, 0u, 0 ) );
                sgC3DObjectPtr pUpperFace = makeSharedGSPtr< sgC3DObject >(
                    (sgC3DObject*)sgSurfaces::Face( *pContour2, 0u, 0 ) );
                ASSERT( pLowerFace && pUpperFace );
                const sgC3DObject* pObjects[] = { pSurface.get(), pLowerFace.get(), pUpperFace.get() };
                
                sgC3DObject* pArea = (sgC3DObject*)sgSurfaces::SewSurfaces( pObjects, 3 );
                ASSERT( pArea );
                ASSERT( pArea->Get3DObjectType() == SG_BODY );
                result.additions.push_back( pArea );
            }
            else
            {
                //area
                Blueprint::Site::FloatPairVector contour;
                pSite->getContour( contour );

                typedef std::vector< const sgC2DObject* > sgCContourVector;
                typedef std::vector< boost::shared_ptr< sgCContour > > ContourPtrVector;
                sgCContourVector contours;
                ContourPtrVector contourPtrs;
                std::vector< double > params;

                boost::shared_ptr< sgCContour > pBottomContour, pTopContour;
                if( pSite->isConnection() )
                {
                    pBottomContour = constructContour( contour, -1.0,            dScale, fMinX, fMinY, -0.3 );
                    pTopContour = constructContour( contour, dHeight * 0.75,   dScale, fMinX, fMinY, -0.3 );
                    contours.push_back( pBottomContour.get() );  params.push_back( 0.0 );
                    contours.push_back( pTopContour.get() );  params.push_back( 0.0 );
                }
                else
                {
                    pBottomContour = constructContour( contour, -1.0,            dScale, fMinX, fMinY, -0.05 );
                    boost::shared_ptr< sgCContour > pMidContour = 
                        constructContour( contour, dHeight * 0.65,  dScale, fMinX, fMinY, -0.05 );
                    pTopContour = constructContour( contour, dHeight + 1.0,   dScale, fMinX, fMinY, -3.0 );
                    contours.push_back( pBottomContour.get() );  params.push_back( 0.0 );
                    contours.push_back( pMidContour.get() );  params.push_back( 0.0 );
                    contours.push_back( pTopContour.get() );  params.push_back( 0.0 );
                    contourPtrs.push_back( pMidContour );
                }

                //sgC3DObjectPtr pSurface2 = makeSharedGSPtr< sgC3DObject >(
                //    (sgC3DObject*)sgSurfaces::LinearSurfaceFromSections( *pContour2, *pContour3, 0.0, false ) );

                sgC3DObjectPtr pWallSurface;
                if( contours.size() == 2 )
                {
                    pWallSurface = makeSharedGSPtr< sgC3DObject >(
                        (sgC3DObject*)sgSurfaces::LinearSurfaceFromSections( *pBottomContour, *pTopContour, 0.0, false ) );
                }
                else
                {
                    pWallSurface = makeSharedGSPtr< sgC3DObject >(
                        (sgC3DObject*)sgSurfaces::SplineSurfaceFromSections( 
                            contours.data(), params.data(), contours.size(), false ) );
                }
                ASSERT( pWallSurface );

                sgC3DObjectPtr pLowerFace = makeSharedGSPtr< sgC3DObject >((sgC3DObject*)sgSurfaces::Face( *pBottomContour, 0u, 0 ) );
                sgC3DObjectPtr pUpperFace = makeSharedGSPtr< sgC3DObject >((sgC3DObject*)sgSurfaces::Face( *pTopContour, 0u, 0 ) );
                ASSERT( pLowerFace && pUpperFace );

                //const sgC3DObject* pObjects[] = { pLowerFace.get(), pSurface1.get(), pSurface2.get(), pUpperFace.get() };
                const sgC3DObject* pObjects[] = { pLowerFace.get(), pWallSurface.get(), pUpperFace.get() };
                
                sgC3DObject* pArea = (sgC3DObject*)sgSurfaces::SewSurfaces( pObjects, 3 );
                ASSERT( pArea );
                ASSERT( pArea->Get3DObjectType() == SG_BODY );
                result.subtractions.push_back( pArea );
            }
        }
    }
}

boost::shared_ptr< sgCGroup > generateBlueprint_sgcore( const SGCoreParams& params )
{
    VERIFY_RTE( params.pSite );

    Blueprint::DataBitmap data;
    params.pSite->evaluate( data );

    //determine the bounding rect
    double   fMinx = std::numeric_limits< double >::max(), 
            fMiny = std::numeric_limits< double >::max(), 
            fMaxx = -std::numeric_limits< double >::max(), 
            fMaxy = -std::numeric_limits< double >::max();
    {
        const Blueprint::DataBitmap::Claim::Vector& claims = data.getClaims();
        for( Blueprint::DataBitmap::Claim::Vector::const_iterator 
            i = claims.begin(),
            iEnd = claims.end(); i!=iEnd; ++i )
        {
            const Blueprint::DataBitmap::Claim& c = *i;
        
            fMinx = std::min( fMinx, c.m_x * params.dScale );
            fMiny = std::min( fMiny, c.m_y * params.dScale );
            fMaxx = std::max( fMaxx, (c.m_x + c.m_pBuffer->getWidth()) * params.dScale );
            fMaxy = std::max( fMaxy, (c.m_y + c.m_pBuffer->getHeight()) * params.dScale );
        }

        fMinx -= params.dBorder * params.dScale;
        fMiny -= params.dBorder * params.dScale;
        fMaxx += params.dBorder * params.dScale;
        fMaxy += params.dBorder * params.dScale;
    }
    VERIFY_RTE( fMinx < fMaxx && fMiny < fMaxy );

    sgC3DObject::AutoTriangulate( false, SG_DELAUNAY_TRIANGULATION );

    //collect the additions and subtractions to make
    Result result;
    params.pSite->for_each_recursive( 
        boost::bind( &collectObjects, boost::ref( result ), _1, params.dHeight, params.dScale, fMinx, fMiny ), 
        generics::_not( generics::all() ) );

    if( !params.bDoSubtraction )
    {
        ObjectVector objectsVector( result.subtractions.begin(), result.subtractions.end() );
        boost::shared_ptr< sgCGroup > pGroup =
            makeSharedGSPtr< sgCGroup >( sgCGroup::CreateGroup( objectsVector.data(), objectsVector.size() ) );
        return pGroup;
    }
    else
    {
        //generate the bounding box
        sgCBox* pBox = sgCreateBox( fMaxx - fMinx, fMaxy - fMiny, params.dHeight * params.dScale );

        //perform the subtractions
        Object3DVector resultObjects;
        resultObjects.push_back( pBox );
    
        for( Object3DVector::iterator 
            j = result.subtractions.begin(),
            jEnd = result.subtractions.end(); j!=jEnd; ++j )
        {
            Object3DVector curObjects;
            curObjects.swap( resultObjects );
            for( Object3DVector::iterator 
                i = curObjects.begin(),
                iEnd = curObjects.end(); i!=iEnd; ++i )
            {
                if( sgCGroup* pResult = sgBoolean::Sub( **i, **j ) )
                {
                    collectObjects( makeSharedGSPtr< sgCGroup >( pResult ), resultObjects );
                    sgDeleteObject( *i );
                }
                else
                    resultObjects.push_back( *i );
            }
            sgDeleteObject( *j );
        }

        //perform the additions
    
        for( Object3DVector::iterator 
            j = result.additions.begin(),
            jEnd = result.additions.end(); j!=jEnd; ++j )
        {
            Object3DVector curObjects;
            curObjects.swap( resultObjects );
            bool bUnion = false;
            for( Object3DVector::iterator 
                i = curObjects.begin(),
                iEnd = curObjects.end(); i!=iEnd; ++i )
            {
                if( sgCGroup* pResult = sgBoolean::Union( **i, **j ) )
                {
                    collectObjects( makeSharedGSPtr< sgCGroup >( pResult ), resultObjects );
                    sgDeleteObject( *i );
                    bUnion = true;
                }
                else
                    resultObjects.push_back( *i );
            }
            if( !bUnion )
                resultObjects.push_back( *j );
            else
                sgDeleteObject( *j );
        }

    
        //collect up the result as a group
        ObjectVector objectsVector( resultObjects.begin(), resultObjects.end() );
        boost::shared_ptr< sgCGroup > pGroup =
            makeSharedGSPtr< sgCGroup >( sgCGroup::CreateGroup( objectsVector.data(), objectsVector.size() ) );

        return pGroup;
    }
    
}

}
*/
