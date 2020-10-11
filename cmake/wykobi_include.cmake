
cmake_minimum_required( VERSION 3.1...3.16 )

find_path( WYKOBI_INSTALL_PATH wykobi.hpp PATHS ${BLUEPRINT_THIRD_PARTY_DIR}/wykobi )

set( WYKOBI_INCLUDE_PATH ${WYKOBI_INSTALL_PATH}/src )

function( link_wykobi targetname )
	target_include_directories( ${targetname} PUBLIC ${WYKOBI_INCLUDE_PATH} )
endfunction( link_wykobi )
