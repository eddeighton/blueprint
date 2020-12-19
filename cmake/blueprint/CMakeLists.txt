cmake_minimum_required(VERSION 2.8)

#get boost
include( ${BLUEPRINT_ROOT_DIR}/cmake/boost_include.cmake )

#get cgal
include( ${BLUEPRINT_ROOT_DIR}/cmake/cgal_include.cmake )

#get agg
include( ${BLUEPRINT_ROOT_DIR}/cmake/agg_include.cmake )

#get common
include( ${BLUEPRINT_ROOT_DIR}/cmake/common_include.cmake )

#get ed
include( ${BLUEPRINT_ROOT_DIR}/cmake/ed_include.cmake )

set( BLUEPRINT_API
    ${BLUEPRINT_API_DIR}/blueprint/basicFeature.h
    ${BLUEPRINT_API_DIR}/blueprint/blueprint.h
    ${BLUEPRINT_API_DIR}/blueprint/buffer.h
    ${BLUEPRINT_API_DIR}/blueprint/cgalSettings.h
    ${BLUEPRINT_API_DIR}/blueprint/cgalUtils.h
    ${BLUEPRINT_API_DIR}/blueprint/clip.h
    ${BLUEPRINT_API_DIR}/blueprint/compilation.h
    ${BLUEPRINT_API_DIR}/blueprint/connection.h
    ${BLUEPRINT_API_DIR}/blueprint/dataBitmap.h
    ${BLUEPRINT_API_DIR}/blueprint/editBase.h
    ${BLUEPRINT_API_DIR}/blueprint/editInteractions.h
    ${BLUEPRINT_API_DIR}/blueprint/editMain.h
    ${BLUEPRINT_API_DIR}/blueprint/editNested.h
    ${BLUEPRINT_API_DIR}/blueprint/factory.h
    ${BLUEPRINT_API_DIR}/blueprint/geometry.h
    ${BLUEPRINT_API_DIR}/blueprint/glyph.h
    ${BLUEPRINT_API_DIR}/blueprint/glyphSpec.h
    ${BLUEPRINT_API_DIR}/blueprint/glyphSpecProducer.h
    ${BLUEPRINT_API_DIR}/blueprint/markup.h
    ${BLUEPRINT_API_DIR}/blueprint/node.h
    ${BLUEPRINT_API_DIR}/blueprint/object.h
    ${BLUEPRINT_API_DIR}/blueprint/property.h
    ${BLUEPRINT_API_DIR}/blueprint/rasteriser.h
    ${BLUEPRINT_API_DIR}/blueprint/serialisation.h
    ${BLUEPRINT_API_DIR}/blueprint/site.h
    ${BLUEPRINT_API_DIR}/blueprint/space.h
    ${BLUEPRINT_API_DIR}/blueprint/spacePolyInfo.h
    ${BLUEPRINT_API_DIR}/blueprint/toolbox.h
    ${BLUEPRINT_API_DIR}/blueprint/transform.h
    ${BLUEPRINT_API_DIR}/blueprint/visibility.h
    ${BLUEPRINT_API_DIR}/blueprint/wall.h
    )
    
    
set( BLUEPRINT_SOURCES_API 
        ${BLUEPRINT_API} 
        )

set( BLUEPRINT_SOURCES_SRC
    ${BLUEPRINT_SRC_DIR}/blueprint/basicFeature.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/blueprint.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/cgalUtils.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/clip.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/compilation.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/compilationGetPolyInfo.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/connection.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/dataBitmap.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/editBase.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/editInteractions.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/editMain.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/editNested.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/factory.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/glyph.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/node.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/object.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/property.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/site.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/space.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/spacePolyInfo.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/svgUtils.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/toolbox.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/visibility.cpp
    ${BLUEPRINT_SRC_DIR}/blueprint/wall.cpp
    )

set( BLUEPRINT_SOURCES ${BLUEPRINT_SOURCES_API} ${BLUEPRINT_SOURCES_SRC} )

source_group( src FILES ${BLUEPRINT_SOURCES} )

add_library( blueprintlib STATIC 
        ${AGG_SRC}
		${BLUEPRINT_SOURCES} 
		)
		
set_target_properties( blueprintlib PROPERTIES FOLDER blueprintlib )
set_target_properties( blueprintlib PROPERTIES DEBUG_POSTFIX d )

target_compile_options( blueprintlib PRIVATE /bigobj )

link_boost( blueprintlib filesystem )
link_boost( blueprintlib iostreams )
link_boost( blueprintlib serialization )
link_cgal( blueprintlib )
link_agg( blueprintlib )
link_common( blueprintlib )
link_ed( blueprintlib )
link_agg( blueprintlib )

install( FILES ${BLUEPRINT_API} DESTINATION include/blueprint )
install( FILES ${BLUEPRINT_CORE_API} DESTINATION include/core )
install( FILES ${BLUEPRINT_MESH_API} DESTINATION include/mesh )
install( FILES ${BLUEPRINT_SPACES_API} DESTINATION include/spaces )
install( FILES ${BLUEPRINT_SEARCH_API} DESTINATION include/search )
install( TARGETS blueprintlib EXPORT blueprint DESTINATION lib)
install( EXPORT blueprint NAMESPACE Blueprint:: FILE blueprint-config.cmake DESTINATION share )