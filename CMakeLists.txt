cmake_minimum_required(VERSION 3.16)
project(dis8 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W3)
else()
  add_compile_options(-Wall -Wextra -Werror)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_library(dis_port STATIC src/dis_port.c)
target_include_directories(dis_port PUBLIC src)

function(dis_tool name)
  add_executable(${name} src/${name}.c)
  target_link_libraries(${name} PRIVATE dis_port)
  target_include_directories(${name} PRIVATE src)
endfunction()

dis_tool(dis8)
dis_tool(disz80)
dis_tool(dis6800)
dis_tool(comtohex)
dis_tool(hextocom)
dis_tool(plus)

enable_testing()
add_test(
  NAME regression
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/regression.py
          $<TARGET_FILE:dis8>
          $<TARGET_FILE:disz80>
          $<TARGET_FILE:dis6800>
          $<TARGET_FILE:comtohex>
          $<TARGET_FILE:hextocom>
          $<TARGET_FILE:plus>
)
set_tests_properties(regression PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
