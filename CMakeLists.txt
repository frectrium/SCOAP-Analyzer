# Set the minimum required version of CMake for this project.
cmake_minimum_required(VERSION 3.10)

# Define the project name. This will be the name of the solution/project file.
project(VerilogScoapAnalyzer)

# Set the C++ standard to C++17 and require it.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use the file(GLOB ...) command to automatically find all .cpp files
# in the 'src' directory. This saves you from having to list each file manually.
file(GLOB SOURCES "src/*.cpp")

# Define the executable target. The first argument is the name of the
# executable that will be created (e.g., 'analyzer.exe' or './analyzer').
# The second argument is the list of source files to compile.
add_executable(analyzer ${SOURCES})

# Add platform-specific dependencies.
# The original code uses the Windows API (<windows.h>) for creating directories.
# This block ensures that on Windows systems, the program links against
# the necessary 'kernel32' library. This block is ignored on other
# operating systems like Linux or macOS, allowing for cross-platform builds.
if(WIN32)
  target_link_libraries(analyzer kernel32)
endif()

# Optional: Add an install command to place the executable in a 'bin' directory.
install(TARGETS analyzer DESTINATION bin)
