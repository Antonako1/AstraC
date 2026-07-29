# CMakeList.txt : CMake project for AstraC, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("AstraC" C CXX)

# Generate resource file with icon path
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/ASTRAC.rc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ASTRAC.rc"
  @ONLY
)

# Add source to this project's executable.
add_executable (AstraC "ASTRAC.c" "ASTRAC.h" "${CMAKE_CURRENT_BINARY_DIR}/ASTRAC.rc")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET AstraC PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
