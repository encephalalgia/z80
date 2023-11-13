project(Z80_test)

add_executable(${PROJECT_NAME} hello_world.cpp)
target_link_libraries(${PROJECT_NAME}
        PRIVATE GTest::gtest_main)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

include(CTest)
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})