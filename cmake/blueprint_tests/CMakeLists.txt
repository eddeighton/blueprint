cmake_minimum_required(VERSION 2.8)

#get boost
include( ${BLUEPRINT_ROOT_DIR}/cmake/boost_include.cmake )

#get cgal
include( ${BLUEPRINT_ROOT_DIR}/cmake/cgal_include.cmake )

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

#get gtest
include( ${BLUEPRINT_ROOT_DIR}/cmake/gtest_include.cmake )

#get the tests
set( BLUEPRINT_TESTS_SOURCE
    ${BLUEPRINT_ROOT_DIR}/tests/blueprintTests.cpp )
    
set( BLUEPRINT_TEST_FILES
    ${BLUEPRINT_ROOT_DIR}/tests/testfiles/basic.blu
    ${BLUEPRINT_ROOT_DIR}/tests/testfiles/basic_child.blu
    ${BLUEPRINT_ROOT_DIR}/tests/testfiles/basic_connection.blu 
    ${BLUEPRINT_ROOT_DIR}/tests/testfiles/basic_loop.blu
    ${BLUEPRINT_ROOT_DIR}/tests/testfiles/basic_exterior_connection.blu  )
    

add_executable( blueprint_tests
					${BLUEPRINT_TESTS_SOURCE}
                    ${AGG_SRC}
				)

##configure all debug builds to have 'd' postfix
set_target_properties( blueprint_tests PROPERTIES DEBUG_POSTFIX d )
set_target_properties( blueprint_tests PROPERTIES FOLDER blueprint_tests )

link_boost( blueprint_tests filesystem )
link_boost( blueprint_tests iostreams )
link_boost( blueprint_tests serialization )
link_cgal( blueprint_tests )
link_agg( blueprint_tests )
link_wykobi( blueprint_tests )
link_clipper( blueprint_tests )
link_common( blueprint_tests )
link_ed( blueprint_tests )

target_link_libraries( blueprint_tests PUBLIC blueprintlib )
    
link_gtest( blueprint_tests )
link_gtest_main( blueprint_tests )

install( TARGETS blueprint_tests DESTINATION bin)
install( FILES ${BLUEPRINT_TEST_FILES} DESTINATION testfiles )
