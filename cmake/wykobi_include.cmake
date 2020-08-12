
cmake_minimum_required( VERSION 3.1...3.16 )

find_path( WYKOBI_INSTALL_PATH wykobi.hpp PATHS ${BLUEPRINT_THIRD_PARTY_DIR}/wykobi )

set( WYKOBI_INCLUDE_PATH ${WYKOBI_INSTALL_PATH}/src )

function( link_wykobi targetname )
	target_include_directories( ${targetname} PRIVATE ${WYKOBI_INCLUDE_PATH} )
	#target_link_libraries( ${targetname} PUBLIC ED::edlib )
endfunction( link_wykobi )
