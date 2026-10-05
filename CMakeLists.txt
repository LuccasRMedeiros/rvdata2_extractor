cmake_minimum_required(VERSION 3.16)

project(rvdata2_extractor
	VERSION 0.1.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit build/compile_commands.json for ccls / IDE navigation.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FindDependencies)

add_executable(rvdata2_extractor
	src/main.cpp
	src/marshal.cpp
	src/zlib_util.cpp
	src/fs_util.cpp
	src/scripts_extract.cpp
)

target_include_directories(rvdata2_extractor
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(rvdata2_extractor
	PRIVATE
		ZLIB::ZLIB
)

# Place the runnable binary in the repo root so it can find Scripts.rvdata2
# next to the source tree without copying from build/.
set_target_properties(rvdata2_extractor PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
