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
	SET(CMAKE_POSITION_INDEPENDENT_CODE ON) # temporary hack to use spdlog as a static library
	add_subdirectory(source/spdlog)
endif()

###### LIBRARY ######
add_library(taskete SHARED
 "source/taskete/memory_resource.cpp"
 "source/taskete/execution_payload.cpp")

target_include_directories(taskete PRIVATE include)
target_compile_features(taskete PRIVATE cxx_std_17)

if(USE_SPDLOG)
	target_compile_definitions(taskete PRIVATE TASKETE_HAS_SPDLOG)
	target_include_directories(taskete PRIVATE source/spdlog/include)
	target_link_libraries(taskete PRIVATE spdlog::spdlog)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(taskete PRIVATE
		-m64

		-Wall
		-Wextra
		-Werror
		-pedantic-errors
		-Wmost	   # https://releases.llvm.org/10.0.0/tools/clang/docs/DiagnosticsReference.html#wmost
		-Wpedantic # https://releases.llvm.org/10.0.0/tools/clang/docs/DiagnosticsReference.html#wpedantic
		-Watomic-implicit-seq-cst
		-Wconversion
		-Wold-style-cast
	)
else() # MSVC
	target_compile_definitions(taskete PRIVATE _CRT_SECURE_NO_WARNINGS)
	target_compile_options(taskete PRIVATE /W4 /WX /permissive-)
endif()


###### TESTS ######
if(RUN_TESTS)

	enable_testing()

	set(TEST_SRC
		"test/test_main.cpp"
		 "test/test_execution_payload.cpp")

	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

		set(CLANG_COMPILER_FLAGS -m64 -fno-omit-frame-pointer -fno-optimize-sibling-calls)
		
		message("Building tests on Clang with various sanitizers...")

		message("[CLANG] Address Sanitizer")
		add_executable(taskete_test-runner ${TEST_SRC})
		target_compile_features(taskete_test-runner PRIVATE cxx_std_17)

		target_include_directories(taskete_test-runner PRIVATE test)

		target_compile_options(taskete_test-runner PRIVATE ${CLANG_COMPILER_FLAGS})

		add_test(NAME RunAllTests COMMAND taskete_test-runner)

	else() # MSVC

		message("Building tests on MSVC...")

		add_executable(taskete_test-runner ${TEST_SRC})
		target_include_directories(taskete_test-runner PRIVATE test)
		target_compile_features(taskete_test-runner PRIVATE cxx_std_17)
		target_compile_definitions(taskete_test-runner PRIVATE _CRT_SECURE_NO_WARNINGS)

		add_test(NAME RunAllTests COMMAND taskete_test-runner)
	endif()
endif()
