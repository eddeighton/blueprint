cmake_minimum_required(VERSION 2.8)

#get boost
include( ${BLUEPRINT_ROOT_DIR}/cmake/boost_include.cmake )

#get agg
include( ${BLUEPRINT_ROOT_DIR}/cmake/agg_include.cmake )

#get wykobi
include( ${BLUEPRINT_ROOT_DIR}/cmake/wykobi_include.cmake )

#get clipper
include( ${BLUEPRINT_ROOT_DIR}/cmake/clipper_include.cmake )

#get common
include( ${BLUEPRINT_ROOT_DIR}/cmake/common_include.cmake )

#get ed
include( ${BLUEPRINT_ROOT_DIR}/cmake/ed_include.cmake )

set( BLUEPRINT_API
    ${BLUEPRINT_API_DIR}/blueprint/buffer.h
    ${BLUEPRINT_API_DIR}/blueprint/dataBitmap.h
    ${BLUEPRINT_API_DIR}/blueprint/edit.h
    ${BLUEPRINT_API_DIR}/blueprint/factory.h
    ${BLUEPRINT_API_DIR}/blueprint/glyph.h
    ${BLUEPRINT_API_DIR}/blueprint/glyphSpec.h
    ${BLUEPRINT_API_DIR}/blueprint/node.h
    ${BLUEPRINT_API_DIR}/blueprint/property.h
    ${BLUEPRINT_API_DIR}/blueprint/site.h
    ${BLUEPRINT_API_DIR}/blueprint/toolbox.h
    )
    
set( BLUEPRINT_CORE_API
    ${BLUEPRINT_API_DIR}/core/rasteriser.h
    ${BLUEPRINT_API_DIR}/core/serialisation.h
    )
    
set( BLUEPRINT_MESH_API
    ${BLUEPRINT_API_DIR}/mesh/osgMesh.h
    ${BLUEPRINT_API_DIR}/mesh/polyvoxGenerator.h
    ${BLUEPRINT_API_DIR}/mesh/sqcoreGenerator.h
    )
    
set( BLUEPRINT_SPACES_API
    ${BLUEPRINT_API_DIR}/spaces/basicarea.h
    ${BLUEPRINT_API_DIR}/spaces/basicFeature.h
    ${BLUEPRINT_API_DIR}/spaces/blueprint.h
    ${BLUEPRINT_API_DIR}/spaces/spaceUtils.h
    )
    
set( BLUEPRINT_SEARCH_API
    ${BLUEPRINT_API_DIR}/search/adjacency.h
    ${BLUEPRINT_API_DIR}/search/search.h
    )
    
set( BLUEPRINT_SOURCES_API 
        ${BLUEPRINT_API} 
        ${BLUEPRINT_CORE_API} 
        ${BLUEPRINT_MESH_API} 
        ${BLUEPRINT_SPACES_API} 
        ${BLUEPRINT_SEARCH_API} 
        )

set( BLUEPRINT_SOURCES_SRC
    ${BLUEPRINT_SRC_DIR}/core/dataBitmap.cpp
    ${BLUEPRINT_SRC_DIR}/core/node.cpp
    ${BLUEPRINT_SRC_DIR}/core/property.cpp
    ${BLUEPRINT_SRC_DIR}/core/site.cpp

    ${BLUEPRINT_SRC_DIR}/edit/edit.cpp
    ${BLUEPRINT_SRC_DIR}/edit/glyph.cpp
    ${BLUEPRINT_SRC_DIR}/edit/toolbox.cpp

    ${BLUEPRINT_SRC_DIR}/mesh/osgMesh.cpp
    ${BLUEPRINT_SRC_DIR}/mesh/polyvoxGenerator.cpp
    ${BLUEPRINT_SRC_DIR}/mesh/sqcoreGenerator.cpp

    ${BLUEPRINT_SRC_DIR}/spaces/basicarea.cpp
    ${BLUEPRINT_SRC_DIR}/spaces/basicFeature.cpp
    ${BLUEPRINT_SRC_DIR}/spaces/blueprint.cpp
    ${BLUEPRINT_SRC_DIR}/spaces/factory.cpp
    ${BLUEPRINT_SRC_DIR}/spaces/spaceUtils.cpp
    )

set( BLUEPRINT_SOURCES ${BLUEPRINT_SOURCES_API} ${BLUEPRINT_SOURCES_SRC} ${AGG_SRC} )

source_group( src FILES ${BLUEPRINT_SOURCES} )

add_library( blueprintlib STATIC 
		${CLIPPER_SOURCE}
		${BLUEPRINT_SOURCES} 
		)
		
set_target_properties( blueprintlib PROPERTIES FOLDER blueprintlib )
set_target_properties( blueprintlib PROPERTIES DEBUG_POSTFIX d )

link_boost( blueprintlib filesystem )
link_boost( blueprintlib iostreams )
link_boost( blueprintlib serialization )
link_agg( blueprintlib )
link_wykobi( blueprintlib )
link_clipper( blueprintlib )
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