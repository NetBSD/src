file(GLOB TESTS "*_test.c")

find_package(CMocka REQUIRED)

foreach (TEST ${TESTS})
    string(REGEX REPLACE ".*/([^/]+).c" "\\1" NAME ${TEST})
    message("Adding test ${NAME}")
    add_executable(${NAME} "${NAME}.c" assertions.c stream_expectations.c)
    target_link_libraries(${NAME} ${CMOCKA_LIBRARIES})
    target_link_libraries(${NAME} cbor)
    target_include_directories(${NAME} PUBLIC ${CMOCKA_INCLUDE_DIR})
    add_test(${NAME} ${NAME})
    add_dependencies(coverage ${NAME})
endforeach (TEST)

add_executable(cpp_linkage_test cpp_linkage_test.cpp)
target_link_libraries(cpp_linkage_test cbor)