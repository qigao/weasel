add_executable(TestResponseParser TestResponseParser.cpp)
target_link_libraries(TestResponseParser PRIVATE WeaselIPC Salts::TinyTest
  Boost::serialization Boost::wserialization)
set_target_properties(TestResponseParser PROPERTIES FOLDER "Tests")
add_test(NAME weasel.response_parser COMMAND TestResponseParser --tap
  --junit "${CMAKE_CURRENT_BINARY_DIR}/response-parser.xml")
set_tests_properties(weasel.response_parser PROPERTIES LABELS "ipc;unit" TIMEOUT 30)
