# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/tfjiang/usr/geo_sim_sdk/branches/jtflib

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/tfjiang/usr/geo_sim_sdk/branches/jtflib-build

# Include any dependencies generated for this target.
include math/CMakeFiles/jtf-math.dir/depend.make

# Include the progress variables for this target.
include math/CMakeFiles/jtf-math.dir/progress.make

# Include the compile flags for this target's objects.
include math/CMakeFiles/jtf-math.dir/flags.make

math/CMakeFiles/jtf-math.dir/src/null.cpp.o: math/CMakeFiles/jtf-math.dir/flags.make
math/CMakeFiles/jtf-math.dir/src/null.cpp.o: /home/tfjiang/usr/geo_sim_sdk/branches/jtflib/math/src/null.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/tfjiang/usr/geo_sim_sdk/branches/jtflib-build/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object math/CMakeFiles/jtf-math.dir/src/null.cpp.o"
	cd /home/tfjiang/usr/geo_sim_sdk/branches/jtflib-build/math && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/jtf-math.dir/src/null.cpp.o -c /home/tfjiang/usr/geo_sim_sdk/branches/jtflib/math/src/null.cpp

math/CMakeFiles/jtf-math.dir/src/null.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/jtf-math.dir/src/null.cpp.i"
	cd /home/tfjiang/usr/geo_sim_sdk/branches/jtflib-build/math && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/tfjiang/usr/geo_sim_sdk/branches/jtflib/math/src/null.cpp > CMakeFiles/jtf-math.dir/src/null.cpp.i

math/CMakeFiles/jtf-math.dir/src/null.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/jtf-math.dir/src/null.cpp.s"
	cd /home/tfjiang/usr/geo_sim_sdk/branches/jtflib-build/math && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/tfjiang/usr/geo_sim_sdk/branches/jtflib/math/src/null.cpp -o CMakeFiles/jtf-math.dir/src/null.cpp.s

math/CMakeFiles/jtf-math.dir/src/null.cpp.o.requires:
.PHONY : math/CMakeFiles/jtf-math.dir/src/null.cpp.o.requires

math/CMakeFiles/jtf-math.dir/src/null.cpp.o.provides: math/CMakeFiles/jtf-math.dir/src/null.cpp.o.requires
	$(MAKE) -f math/CMakeFiles/jtf-math.dir/build.make math/CMakeFiles/jtf-math.dir/src/null.cpp.o.provides.build
.PHONY : math/CMakeFiles/jtf-math.dir/src/null.cpp.o.provides

math/CMakeFiles/jtf-math.dir/src/null.cpp.o.provides.build: math/CMakeFiles/jtf-math.dir/src/null.cpp.o

# Object files for target jtf-math
jtf__math_OBJECTS = \
"CMakeFiles/jtf-math.dir/src/null.cpp.o"

# External object files for target jtf-math
jtf__math_EXTERNAL_OBJECTS =

lib/libjtf-mathd.so: math/CMakeFiles/jtf-math.dir/src/null.cpp.o
lib/libjtf-mathd.so: math/CMakeFiles/jtf-math.dir/build.make
lib/libjtf-mathd.so: math/CMakeFiles/jtf-math.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX shared library ../lib/libjtf-mathd.so"
	cd /home/tfjiang/usr/geo_sim_sdk/branches/jtflib-build/math && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/jtf-math.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
math/CMakeFiles/jtf-math.dir/build: lib/libjtf-mathd.so
.PHONY : math/CMakeFiles/jtf-math.dir/build

math/CMakeFiles/jtf-math.dir/requires: math/CMakeFiles/jtf-math.dir/src/null.cpp.o.requires
.PHONY : math/CMakeFiles/jtf-math.dir/requires

math/CMakeFiles/jtf-math.dir/clean:
	cd /home/tfjiang/usr/geo_sim_sdk/branches/jtflib-build/math && $(CMAKE_COMMAND) -P CMakeFiles/jtf-math.dir/cmake_clean.cmake
.PHONY : math/CMakeFiles/jtf-math.dir/clean

math/CMakeFiles/jtf-math.dir/depend:
	cd /home/tfjiang/usr/geo_sim_sdk/branches/jtflib-build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tfjiang/usr/geo_sim_sdk/branches/jtflib /home/tfjiang/usr/geo_sim_sdk/branches/jtflib/math /home/tfjiang/usr/geo_sim_sdk/branches/jtflib-build /home/tfjiang/usr/geo_sim_sdk/branches/jtflib-build/math /home/tfjiang/usr/geo_sim_sdk/branches/jtflib-build/math/CMakeFiles/jtf-math.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : math/CMakeFiles/jtf-math.dir/depend

