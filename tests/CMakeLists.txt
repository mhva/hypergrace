find_package (GTest)

# Add tests here
set(TESTS
    bencode_intdecoding_test.cc
    bencode_strdecoding_test.cc
    bencode_collectionsdecoding_test.cc
    bitfield_test.cc
    bittorrent_message_test.cc
    delegate_binding_test.cc
    #    fileregistry_test.cc
    http_middleware_test.cc
    packet_framework_test.cc
    rating_test.cc
    time_test.cc
    #    torrent_parse_test.cc
    uri_test.cc
)

if (GTEST_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS}
                        ${CMAKE_SOURCE_DIR}/libhypergrace)

    add_executable(testsuite_runner ${TESTS})
    target_link_libraries(testsuite_runner hypergrace ${GTEST_BOTH_LIBRARIES})
    add_custom_target(test COMMAND "testsuite_runner"
                      COMMENT "Starting test-suite ...")
else ()
    message(STATUS "googletest cannot be found. Test suite will not be built.")
endif ()
