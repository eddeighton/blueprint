
cmake_minimum_required( VERSION 3.1...3.16 )

find_path( CGAL_AUX_INSTALL_PATH include/CGAL PATHS ${GEOMETRY_THIRD_PARTY_DIR}/cgal/downloaded_install )
find_path( CGAL_INSTALL_PATH include/CGAL PATHS ${GEOMETRY_THIRD_PARTY_DIR}/cgal/install )

set( CMAKE_PREFIX_PATH "${CGAL_INSTALL_PATH}/CGAL;${CMAKE_PREFIX_PATH}" )
find_package( cgal )

function( link_cgal targetname )
	target_include_directories( ${targetname} PUBLIC ${CGAL_AUX_INSTALL_PATH}/auxiliary/gmp/include )
	target_include_directories( ${targetname} PUBLIC ${CGAL_INCLUDE_DIRS} )
    
    
    target_link_directories( ${targetname} PRIVATE ${CGAL_LIBRARIES_DIR} )
    
    
    target_link_libraries( ${targetname} PUBLIC ${CGAL_AUX_INSTALL_PATH}/auxiliary/gmp/lib/libgmp-10.lib )
    target_link_libraries( ${targetname} PUBLIC ${CGAL_AUX_INSTALL_PATH}/auxiliary/gmp/lib/libmpfr-4.lib )
    target_link_libraries( ${targetname} PUBLIC ${CGAL_INSTALL_PATH}/lib/CGAL-vc142-mt-5.1.1-I-900.lib )
    
    #target_link_libraries( ${targetname} CGAL:: )
    #target_link_directories( ${targetname} PRIVATE ${CGAL_LIBRARIES_DIR} )
	#target_include_directories( ${targetname} PUBLIC ${CGAL_INSTALL_PATH}/include )
    #target_compile_definitions( ${targetname} PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE )
	#target_include_directories( ${targetname} PUBLIC ${CGAL_INSTALL_PATH}/include )
    
    
endfunction( link_cgal )

