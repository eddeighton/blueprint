
#ifndef COMPILATION_15_DEC_2020
#define COMPILATION_15_DEC_2020

#include "blueprint/geometry.h"
#include "blueprint/transform.h"
#include "blueprint/space.h"
#include "blueprint/spacePolyInfo.h"
#include "blueprint/cgalSettings.h"

#include "boost/shared_ptr.hpp"
#include "boost/filesystem/path.hpp"

#include <memory>
#include <map>

namespace Blueprint
{
    class Blueprint;
    
    class Compilation
    {
    public:
        Compilation( boost::shared_ptr< Blueprint > pBlueprint );
        
        static void renderContour( Arrangement& arr, const Transform& transform, 
            Polygon poly, int iOrientation );
        
        using FaceHandle = Arrangement::Face_const_handle;
        using FaceHandleSet = std::set< FaceHandle >;
        void getFaces( FaceHandleSet& floorFaces, FaceHandleSet& fillerFaces );
        
        //using SpacePolyMap = std::map< Space::Ptr, SpacePolyInfo::Ptr >;
        //void getSpacePolyMap( SpacePolyMap& spacePolyMap );
        
        //html svg utilities
        void render( const boost::filesystem::path& filepath );
        void renderFloors( const boost::filesystem::path& filepath );
        void renderFillers( const boost::filesystem::path& filepath );
        
    private:
        
        void recurse( Site::Ptr pSpace );
        void recursePost( Site::Ptr pSpace );
        void connect( Site::Ptr pConnection );
        
        
        void findSpaceFaces( Space::Ptr pSpace, FaceHandleSet& faces, FaceHandleSet& spaceFaces );
        //void recursePolyMap( Site::Ptr pSite, SpacePolyMap& spacePolyMap, 
        //    FaceHandleSet& floorFaces, FaceHandleSet& fillerFaces );
        
        Arrangement m_arr;
    };

    


}

#endif //COMPILATION_15_DEC_2020