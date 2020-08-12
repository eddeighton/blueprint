
#include <windows.h>

#include <iostream>

#include <boost/program_options.hpp>

#include "common/log.h"
#include "common/tick.h"

#include "sgCore.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Vec3>
#include <osg/ShapeDrawable>

#include <osgViewer/Viewer>
#include <osgWidget/WindowManager>
#include <osgGA/TrackballManipulator>
#include <osgGA/FirstPersonManipulator>
#include <osgGA/UFOManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/StateSetManipulator>

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "blueprint\factory.h"

#include "mesh\polyvoxGenerator.h"
#include "mesh\sqcoreGenerator.h"
#include "mesh\osgMesh.h"

void renderLoop( osgViewer::Viewer& viewer )
{
    Clock m_clock;
    Tick m_lastTick = m_clock(), m_startTick = m_clock();
    unsigned int uiLastSecond = 0u;
    unsigned int uiFrames = 0u;
    
    while( viewer.isRealized() )
    {
        ++uiFrames;
        const Tick uiTick = m_clock();
        float dt = FloatTickDuration(uiTick - m_lastTick).count();
        float ct = FloatTickDuration(uiTick - m_startTick).count();
        viewer.frame( ct );
        m_lastTick = uiTick;
        const unsigned int uiSecond = static_cast< unsigned int >( ct );
        /*if( uiSecond > uiLastSecond )
        {
            uiLastSecond = uiSecond;
            LOG( info ) << "fps: " << uiFrames;
            uiFrames = 0u;
        }*/
    }
}

int main(int argc, char ** argv)
{
    LoggingSystem logSystem( argv[ 0 ] );
    
    std::string strBlueprintFilePath, strGenerationType, strTextureFilePath, strTestModelFile, strFileOutput;
    double dBorder = 16, dHeight = 4,  dScale = 1.0;
    bool bSmooth = false, bDoSubtract = true;
    
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("file", boost::program_options::value< std::string >( &strBlueprintFilePath ), "File path to blueprint file (*.blu)")
        ("model", boost::program_options::value< std::string >( &strTestModelFile ), "Model file to test" )
        ("output", boost::program_options::value< std::string >( &strFileOutput ), "Model file to generate" )
        ("tex", boost::program_options::value< std::string >( &strTextureFilePath ), "Texture file path")
        ("type", boost::program_options::value< std::string >( &strGenerationType ), "Type of mesh generation (polyvox|sqcore)" )
        ("height", boost::program_options::value< double >( &dHeight ), "Height of generated mesh" )
        ("border", boost::program_options::value< double >( &dBorder ), "Border size around generated mesh" )
        ("smooth", boost::program_options::value< bool >( &bSmooth ), "Whether polyvox generation is smooth or cubic" )
        ("scale", boost::program_options::value< double >( &dScale ), "Mesh scaling" )
        ("subtract", boost::program_options::value< bool >( &bDoSubtract ), "SGCore subtractions" )
    ;
    
    boost::program_options::variables_map vm;
    boost::program_options::store( boost::program_options::parse_command_line( argc, argv, desc), vm);
    boost::program_options::notify( vm );    

    if ( vm.count("help") || strBlueprintFilePath.empty() ) {
        std::cout << desc << "\n";
        return 1;
    }

    Blueprint::Site::Ptr pSite;
    
    try
    {
        Blueprint::Factory factory;
        pSite = factory.load( strBlueprintFilePath );
    }
    catch( std::runtime_error& e )
    {
        std::cout << "Error loading file: " << strBlueprintFilePath << " : " << e.what();
    }

    //osg::ref_ptr< osg::Geode > pResult;
    osg::ref_ptr< osg::Node > pResult;

    if( strTestModelFile.empty() )
    {
        if( strGenerationType == "sgcore" )
        {
            VERIFY_RTE( sgInitKernel() );
            {
                boost::shared_ptr< sgCGroup > pSGResult =
                    Generator::generateBlueprint_sgcore( 
                        Generator::SGCoreParams( pSite, dHeight, dBorder, dScale, bDoSubtract ) );
                pResult = Generator::createQGCoreMesh( pSGResult );
            }
            sgFreeKernel();
        }
        else
        {
            PolyVox::SurfaceMesh< PolyVox::PositionMaterialNormal > mesh;
            Generator::generateBlueprint( pSite, mesh, dHeight, dBorder, bSmooth );
            pResult = Generator::polyVoxToOsgGeometry( mesh );
        }
        if( !strFileOutput.empty() )
            osgDB::writeObjectFile( *pResult, strFileOutput.c_str() );
    }
    else
    {
        pResult = osgDB::readNodeFile( strTestModelFile.c_str() );
    }
    
    if( pResult )
    {
        LOG( info ) << "Generated mesh for blueprint: " << strBlueprintFilePath;

        osgViewer::Viewer viewer;
        osgWidget::WindowManager windowManager( &viewer, 640, 640, 0xF0000000 );
        osg::ref_ptr< osg::Group > pGroup( new osg::Group );
        
        if( !strTextureFilePath.empty() )
        {
            osg::ref_ptr< osg::Texture2D > pTexture = new osg::Texture2D;
            pTexture->setDataVariance( osg::Object::DYNAMIC );
            osg::Image* pImage = osgDB::readImageFile( strTextureFilePath.c_str() );
            VERIFY_RTE_MSG( pImage, "Failed to load image: " << strTextureFilePath );
            pTexture->setImage( pImage );
            pTexture->setWrap( osg::Texture::WRAP_S, osg::Texture::REPEAT );
            pTexture->setWrap( osg::Texture::WRAP_T, osg::Texture::REPEAT );
            osg::ref_ptr< osg::StateSet > pStateSet = new osg::StateSet;
            pStateSet->setTextureAttributeAndModes( 0, pTexture.get(), osg::StateAttribute::ON );
            pGroup->setStateSet( pStateSet.get() );
        }

        pGroup->addChild( pResult );
        viewer.setSceneData( pGroup.get() );
        viewer.addEventHandler( new osgGA::StateSetManipulator( viewer.getCamera()->getOrCreateStateSet()) );
        viewer.setUpViewInWindow( 100, 100, 740, 740 );
        viewer.setThreadingModel( osgViewer::ViewerBase::ThreadingModel::SingleThreaded );
        //viewer.setCameraManipulator( new osgGA::TrackballManipulator );
        viewer.setCameraManipulator( new osgGA::TerrainManipulator );
        windowManager.resizeAllWindows();
        viewer.realize();   

        renderLoop( viewer );
    }
    

    return 0;
}