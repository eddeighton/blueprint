cmake_minimum_required(VERSION 2.8)

#find path is not working...
set( COMMON_INSTALL_PATH ${BLUEPRINT_ROOT_DIR}/../../common/install/ )
message( "Warning - using temporary hack to auto discover common installation" )

include( ${COMMON_INSTALL_PATH}/share/common-config.cmake )

#find_package( common )

function( link_common targetname )
	target_link_libraries( ${targetname} PUBLIC Common::commonlib )
endfunction( link_common )
