add_executable(tsf_com_server_test tsf_com_server_test.cpp)
add_executable(tsf_host_filter_test tsf_host_filter_test.cpp)
target_link_libraries(tsf_com_server_test PRIVATE ole32 uuid)
foreach(_test IN ITEMS tsf_com_server_test tsf_host_filter_test)
  target_compile_options(${_test} PRIVATE /W4 /WX)
  add_dependencies(${_test} WeaselTSF)
  set_target_properties(${_test} PROPERTIES FOLDER "Tests")
endforeach()
add_test(NAME weasel.tsf_com_server
  COMMAND tsf_com_server_test "$<TARGET_FILE:WeaselTSF>")
add_test(NAME weasel.tsf_host_filter
  COMMAND tsf_host_filter_test "$<TARGET_FILE:WeaselTSF>")
set_tests_properties(weasel.tsf_com_server weasel.tsf_host_filter
  PROPERTIES LABELS "tsf;native" TIMEOUT 60)
