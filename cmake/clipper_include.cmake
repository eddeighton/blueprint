
cmake_minimum_required( VERSION 3.1...3.16 )

find_path( CLIPPER_INSTALL_PATH clipper.hpp PATHS ${BLUEPRINT_THIRD_PARTY_DIR}/clipper/src )

set( CLIPPER_INCLUDE_PATH ${CLIPPER_INSTALL_PATH}/cpp )
set( CLIPPER_SOURCE ${CLIPPER_INCLUDE_PATH}/clipper.hpp ${CLIPPER_INCLUDE_PATH}/clipper.cpp )

function( link_clipper targetname )
	target_include_directories( ${targetname} PUBLIC ${CLIPPER_INCLUDE_PATH} )
endfunction( link_clipper )
