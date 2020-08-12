#include "mesh/polyvoxGenerator.h"
/*
#include "PolyVoxCore/CubicSurfaceExtractorWithNormals.h"
#include "PolyVoxCore/MarchingCubesSurfaceExtractor.h"
#include "PolyVoxCore/SurfaceMesh.h"
#include "PolyVoxCore/SimpleVolume.h"

#include "common\edsMath.h"

#include "blueprint\databitmap.h"

namespace Generator
{
    
void createSphereInVolume( PolyVox::SimpleVolume<uint8_t>& volData, float fRadius)
{
    using namespace PolyVox;
	//This vector hold the position of the center of the volume
	Vector3DFloat v3dVolCenter(volData.getWidth() / 2, volData.getHeight() / 2, volData.getDepth() / 2);

	//This three-level for loop iterates over every voxel in the volume
	for (int z = 0; z < volData.getDepth(); z++)
	{
		for (int y = 0; y < volData.getHeight(); y++)
		{
			for (int x = 0; x < volData.getWidth(); x++)
			{
				//Store our current position as a vector...
				Vector3DFloat v3dCurrentPos(x,y,z);	
				//And compute how far the current position is from the center of the volume
				float fDistToCenter = (v3dCurrentPos - v3dVolCenter).length();

				uint8_t uVoxelValue = 0;

				//If the current voxel is less than 'radius' units from the center then we make it solid.
				if(fDistToCenter <= fRadius)
				{
					//Our new voxel value
					uVoxelValue = 255;
				}

				//Wrte the voxel value into the volume	
				volData.setVoxelAt(x, y, z, uVoxelValue);
			}
		}
	}
}

void generateSphere( PolyVox::SurfaceMesh< PolyVox::PositionMaterialNormal >& mesh )
{
    using namespace PolyVox;
    
	//Create an empty volume and then place a sphere in it
	SimpleVolume<uint8_t> volData(PolyVox::Region(Vector3DInt32(0,0,0), Vector3DInt32(63, 63, 63)));
	createSphereInVolume(volData, 30);

	//Create a surface extractor. Comment out one of the following two lines to decide which type gets created.
	CubicSurfaceExtractorWithNormals< SimpleVolume<uint8_t> > surfaceExtractor(&volData, volData.getEnclosingRegion(), &mesh);
	//MarchingCubesSurfaceExtractor< SimpleVolume<uint8_t> > surfaceExtractor(&volData, volData.getEnclosingRegion(), &mesh);

	//Execute the surface extractor.
	surfaceExtractor.execute();

}

void generateBlueprint( Blueprint::Site::Ptr pSite, 
                       PolyVox::SurfaceMesh< PolyVox::PositionMaterialNormal >& mesh,
                       int iHeight, int iBorder, bool bSmooth )
{
    iHeight = std::max( iHeight, 3 );
    iBorder = std::max( iBorder, 1 );

    Blueprint::DataBitmap data;
    pSite->evaluate( data );

    //determine the bounding rect
    float   fMinx = std::numeric_limits< float >::max(), 
            fMiny = std::numeric_limits< float >::max(), 
            fMaxx = std::numeric_limits< float >::min(), 
            fMaxy = std::numeric_limits< float >::min();

    const Blueprint::DataBitmap::Claim::Vector& claims = data.getClaims();
    for( Blueprint::DataBitmap::Claim::Vector::const_iterator 
        i = claims.begin(),
        iEnd = claims.end(); i!=iEnd; ++i )
    {
        const Blueprint::DataBitmap::Claim& c = *i;
        
        fMinx = std::min( fMinx, c.m_x );
        fMiny = std::min( fMiny, c.m_y );
        fMaxx = std::max( fMaxx, c.m_x + c.m_pBuffer->getWidth() );
        fMaxy = std::max( fMaxy, c.m_y + c.m_pBuffer->getHeight() );
    }

    fMinx -= iBorder;
    fMiny -= iBorder;
    fMaxx += iBorder;
    fMaxy += iBorder;

    //allocate the point cloud and generate
    ASSERT( fMinx < fMaxx && fMiny < fMaxy );

    const int iMaxX = EdsMath::roundRealOutToInt( fMaxx - fMinx );
    const int iMaxY = EdsMath::roundRealOutToInt( fMaxy - fMiny );
    
	PolyVox::SimpleVolume< uint8_t > volData(
        PolyVox::Region( 
            PolyVox::Vector3DInt32( 0,  0,  0   ), 
            PolyVox::Vector3DInt32( iMaxX,  iMaxY,  iHeight  )   ));

	for (int y = 0; y < volData.getHeight(); y++)
	{
		for (int x = 0; x < volData.getWidth(); x++)
		{
            const float fX = (float)x + fMinx;
            const float fY = (float)y + fMiny;

            unsigned char uiValue = 255u;
            //determine the point density...
            for( Blueprint::DataBitmap::Claim::Vector::const_iterator 
                i = claims.begin(),
                iEnd = claims.end(); i!=iEnd && uiValue; ++i )
            {
                const Blueprint::DataBitmap::Claim& c = *i;
                if( fX < c.m_x + c.m_pBuffer->getWidth() && fX >= c.m_x )
                {
                    if( fY < c.m_y + c.m_pBuffer->getHeight() && fY >= c.m_y )
                    {
                        if( *c.m_pBuffer->getAt( fX - c.m_x, fY - c.m_y ) )
                            uiValue = 0u;
                    }
                }
            }
                
			volData.setVoxelAt(x, y, 0, 0u );
			volData.setVoxelAt(x, y, volData.getDepth() - 1, 0u );
	        for (int z = 1; z < volData.getDepth() - 1; z++)
				volData.setVoxelAt(x, y, z, uiValue );
        }
    }
    
    using namespace PolyVox;
    if( bSmooth )
    {
	    MarchingCubesSurfaceExtractor< SimpleVolume<uint8_t> > surfaceExtractor(&volData, volData.getEnclosingRegion(), &mesh);
	    surfaceExtractor.execute();
    }
    else
    {
	    CubicSurfaceExtractorWithNormals< SimpleVolume<uint8_t> > surfaceExtractor( &volData, volData.getEnclosingRegion(), &mesh );
	    surfaceExtractor.execute();
    }
}

}
*/
