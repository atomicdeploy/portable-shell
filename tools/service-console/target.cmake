# Compile an opt-in example with its own explicit document capacity. The normal
# core target and embedded consumers retain their independently chosen flags.
add_executable(service_console_checks
  "${root}/tests/service_console_checks.c" "${root}/tests/render.c"
  "${root}/examples/console_workspace.c" "${root}/src/portable_shell.c")
target_include_directories(service_console_checks PRIVATE "${root}/include")
target_compile_features(service_console_checks PRIVATE c_std_99)
target_compile_definitions(service_console_checks PRIVATE PS_EDITOR_CAPACITY=16384
  PS_REPLAY_ONLY PS_REPLAY_ATTRIBUTES W=100 H=34 PS_REPLAY_SVG_CAPACITY=800000)
if(MSVC)
  target_compile_options(service_console_checks PRIVATE /UNDEBUG /W4)
else()
  target_compile_options(service_console_checks PRIVATE -UNDEBUG -Wall -Wextra -Werror)
endif()
set_target_properties(service_console_checks PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set_target_properties(service_console_checks PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/bin")
endforeach()
