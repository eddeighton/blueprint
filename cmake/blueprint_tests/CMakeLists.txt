cmake_minimum_required(VERSION 2.8)

#get boost
include( ${ED_ROOT_DIR}/cmake/boost_include.cmake )

#get common
include( ${ED_ROOT_DIR}/cmake/common_include.cmake )

#get gtest
include( ${ED_ROOT_DIR}/cmake/gtest_include.cmake )

#get ed
include( ${ED_ROOT_DIR}/cmake/ed_include.cmake )

#get the tests

set( ED_TESTS_SOURCE
    
    #common/
    #ed/
    #inheritTestData/
    #meta/
    
    ${ED_ROOT_DIR}/tests/argumentTests.cpp
    ${ED_ROOT_DIR}/tests/basicGrammars.cpp
    ${ED_ROOT_DIR}/tests/codeTests.cpp
    ${ED_ROOT_DIR}/tests/declaratorTests.cpp
    ${ED_ROOT_DIR}/tests/expressionTests.cpp
    ${ED_ROOT_DIR}/tests/fileRefTests.cpp
    ${ED_ROOT_DIR}/tests/fileTests.cpp
    ${ED_ROOT_DIR}/tests/identifierTests.cpp
    ${ED_ROOT_DIR}/tests/nodeioTests.cpp
    ${ED_ROOT_DIR}/tests/nodeTests.cpp
    ${ED_ROOT_DIR}/tests/overrideTests.cpp
    ${ED_ROOT_DIR}/tests/refTests.cpp
    ${ED_ROOT_DIR}/tests/shorthandioTests.cpp
    ${ED_ROOT_DIR}/tests/shorthandTests.cpp
    ${ED_ROOT_DIR}/tests/skipTests.cpp
    ${ED_ROOT_DIR}/tests/statementTests.cpp
    ${ED_ROOT_DIR}/tests/tagTests.cpp
    ${ED_ROOT_DIR}/tests/typeTests.cpp )

add_executable( ed_tests
					${ED_TESTS_SOURCE}
				)

##configure all debug builds to have 'd' postfix
set_target_properties( ed_tests PROPERTIES DEBUG_POSTFIX d )
set_target_properties( ed_tests PROPERTIES FOLDER ed_tests )

link_boost( ed_tests filesystem )
link_boost( ed_tests iostreams )
link_boost( ed_tests serialization )
link_common( ed_tests )
link_ed( ed_tests )
link_gtest( ed_tests )
link_gtest_main( ed_tests )

install( TARGETS ed_tests DESTINATION bin)
