add_executable(test_protocol test_protocol.c)
target_link_libraries(test_protocol PRIVATE ovnc)
target_include_directories(test_protocol PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
