cmake_minimum_required (VERSION 3.10)

project ("Taskete" LANGUAGES CXX)

option(RUN_TESTS "Builds and runs the tests" OFF)
option(USE_SPDLOG "Compiles and links against the provided spdlog library" OFF)

# Fail early if not building with Clang or MSVC
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	message("Unsupported compiler!")
	return()
endif()

if(USE_SPDLOG)
	add_subdirectory(source/spdlog)
endif()

###### LIBRARY ######
add_library(taskete SHARED
	"source/taskete/memory_resource.cpp"
)
target_include_directories(taskete PRIVATE include)

if(USE_SPDLOG)
	target_compile_definitions(taskete PRIVATE TASKETE_HAS_SPDLOG)
	target_include_directories(taskete PRIVATE source/spdlog/include)
	target_link_libraries(taskete PRIVATE spdlog::spdlog)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
else() # MSVC
	target_compile_options(taskete PRIVATE /std:c++17)
endif()
###### ####### ######

###### TESTS ######
if(RUN_TESTS)

	enable_testing()

	set(TEST_SRC
		"test/test_main.cpp"
		)

	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

		set(CLANG_COMPILER_FLAGS -std=c++17 -m64 -fno-omit-frame-pointer -fno-optimize-sibling-calls)
		
		message("Building tests on Clang with various sanitizers...")

		message("[CLANG] Address Sanitizer")
		add_executable(taskete_test-runner ${TEST_SRC})

		target_include_directories(taskete_test-runner PRIVATE test)

		target_compile_options(taskete_test-runner PRIVATE ${CLANG_COMPILER_FLAGS})

		if(USE_SPDLOG)
			target_link_libraries(taskete_test-runner PRIVATE spdlog::spdlog)
		endif()

	else() # MSVC
		
		message("Building tests on MSVC...")

		add_executable(taskete_test-runner ${TEST_SRC})
		target_include_directories(taskete_test-runner PRIVATE test)
		target_compile_options(taskete_test-runner PRIVATE /std:c++17) # tried with cxx_std_17 but didn't work

		if(USE_SPDLOG)
			target_link_libraries(taskete_test-runner PRIVATE spdlog::spdlog)
		endif()
	endif()
endif()
###### ##### ######