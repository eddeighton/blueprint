
cmake_minimum_required( VERSION 3.1...3.16 )

find_path( AGG_INSTALL_PATH include/agg_math.h PATHS ${BLUEPRINT_THIRD_PARTY_DIR}/agg/src/agg-2.4/ )

file( GLOB AGG_HEADERS ${AGG_INSTALL_PATH}/include/*.h )
file( GLOB AGG_SRC ${AGG_INSTALL_PATH}/src/*.cpp )
source_group( agg FILES  ${AGG_HEADERS} ${AGG_SRC} )

function( link_agg targetname )
	target_include_directories( ${targetname} PUBLIC ${AGG_INSTALL_PATH}/include )
endfunction( link_agg )
