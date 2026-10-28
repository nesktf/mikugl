file(GLOB SHADER_FILES "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag")
set(CONTENT "#pragma once\n\n#include <string_view>\n\nnamespace mikugl::shaders {\n\n")

foreach(SHADER_PATH ${SHADER_FILES})
  get_filename_component(FNAME "${SHADER_PATH}" NAME)
  string(REPLACE "." "_" VAR_NAME "${FNAME}")
  file(READ "${SHADER_PATH}" SHADER_SRC)
  string(APPEND CONTENT "inline constexpr std::string_view k_${VAR_NAME} = R\"glsl(\n${SHADER_SRC})glsl\";\n\n")
endforeach()

string(APPEND CONTENT "} // namespace mikugl::shaders\n")

get_filename_component(OUT_DIR "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${OUT_DIR}")
file(WRITE "${OUTPUT_FILE}" "${CONTENT}")
