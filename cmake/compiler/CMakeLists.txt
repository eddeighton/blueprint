
cmake_minimum_required( VERSION 3.1...3.16 )

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

set( COMPILER_SOURCES
	${BLUEPRINT_SRC_DIR}/compiler/command_compile.cpp
	${BLUEPRINT_SRC_DIR}/compiler/command_clean.cpp
	${BLUEPRINT_SRC_DIR}/compiler/command_info.cpp
	${BLUEPRINT_SRC_DIR}/compiler/main.cpp
	${BLUEPRINT_SRC_DIR}/compiler/stacktrace.cpp
	)

source_group( compiler FILES ${COMPILER_SOURCES} )

add_executable( compiler ${COMPILER_SOURCES} )

target_link_libraries( compiler PRIVATE blueprintlib )

set_target_properties( compiler PROPERTIES FOLDER driver )
set_target_properties( compiler PROPERTIES DEBUG_POSTFIX d )

link_boost( compiler filesystem )
link_boost( compiler iostreams )
link_boost( compiler serialization )
link_boost( compiler program_options )
link_boost( compiler timer )
link_cgal( compiler )
link_agg( compiler )
link_common( compiler )
link_ed( compiler )

install( TARGETS compiler DESTINATION bin)