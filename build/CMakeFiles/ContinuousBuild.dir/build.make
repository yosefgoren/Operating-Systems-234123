# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.18

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/student/.local/lib/python2.7/site-packages/cmake/data/bin/cmake

# The command to remove a file.
RM = /home/student/.local/lib/python2.7/site-packages/cmake/data/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/student/hw4/wet

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/student/hw4/wet/build

# Utility rule file for ContinuousBuild.

# Include the progress variables for this target.
include CMakeFiles/ContinuousBuild.dir/progress.make

CMakeFiles/ContinuousBuild:
	/home/student/.local/lib/python2.7/site-packages/cmake/data/bin/ctest -D ContinuousBuild

ContinuousBuild: CMakeFiles/ContinuousBuild
ContinuousBuild: CMakeFiles/ContinuousBuild.dir/build.make

.PHONY : ContinuousBuild

# Rule to build all files generated by this target.
CMakeFiles/ContinuousBuild.dir/build: ContinuousBuild

.PHONY : CMakeFiles/ContinuousBuild.dir/build

CMakeFiles/ContinuousBuild.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ContinuousBuild.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ContinuousBuild.dir/clean

CMakeFiles/ContinuousBuild.dir/depend:
	cd /home/student/hw4/wet/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/student/hw4/wet /home/student/hw4/wet /home/student/hw4/wet/build /home/student/hw4/wet/build /home/student/hw4/wet/build/CMakeFiles/ContinuousBuild.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ContinuousBuild.dir/depend

