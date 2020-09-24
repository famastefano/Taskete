cmake_minimum_required (VERSION 3.10)

project ("Taskete" LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS NO)

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
 "source/taskete/execution_payload.cpp"
 "source/taskete/node.cpp"
 "source/taskete/shared_memory.cpp")

target_include_directories(taskete PRIVATE include)
target_compile_features(taskete PRIVATE cxx_std_17)
target_compile_definitions(taskete PRIVATE TASKETE_EXPORT_SYMBOLS)

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
	string(REPLACE "/W3" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # hack to remove D9025 warning regarding overriding /W3 with /W4

	target_compile_definitions(taskete PRIVATE _CRT_SECURE_NO_WARNINGS)
	target_compile_options(taskete PRIVATE
	/W4
	/WX
	/permissive-
	/wd4251 # 'member' needs to have dll interface
	)
endif()

###### TESTS ######
if(BUILD_TESTING)
	enable_testing()

	set(TEST_SRC
		"test/test_main.cpp"
		"test/test_execution_payload.cpp"
		"test/test_shared_memory.cpp")

	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

		set(CLANG_COMPILER_FLAGS -O1 -m64 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize-blacklist=${PROJECT_SOURCE_DIR}/blacklist.txt)
		
		message("Configuring tests for Clang with various sanitizers...")
		
		# Base Test Runner
		add_executable(taskete_test-runner ${TEST_SRC})
		target_compile_features(taskete_test-runner PRIVATE cxx_std_17)
		target_include_directories(taskete_test-runner PRIVATE test)
		target_compile_options(taskete_test-runner PRIVATE ${CLANG_COMPILER_FLAGS})
		target_link_libraries(taskete_test-runner PRIVATE taskete)
		######

		# Undefined Behaviour Sanitizer
		add_executable(taskete_test-runner_UBSAN ${TEST_SRC})
		target_compile_features(taskete_test-runner_UBSAN PRIVATE cxx_std_17)
		target_include_directories(taskete_test-runner_UBSAN PRIVATE test)
		target_compile_options(taskete_test-runner_UBSAN PRIVATE ${CLANG_COMPILER_FLAGS} -fsanitize=undefined)
		target_link_libraries(taskete_test-runner_UBSAN PRIVATE taskete -fsanitize=undefined)
		######

		# Address Sanitizer
		add_executable(taskete_test-runner_ASAN ${TEST_SRC})
		target_compile_features(taskete_test-runner_ASAN PRIVATE cxx_std_17)
		target_include_directories(taskete_test-runner_ASAN PRIVATE test)
		target_compile_options(taskete_test-runner_ASAN PRIVATE ${CLANG_COMPILER_FLAGS} -fsanitize=address)
		target_link_libraries(taskete_test-runner_ASAN PRIVATE taskete -fsanitize=address)
		######

		# Thread Sanitizer
		add_executable(taskete_test-runner_THSAN ${TEST_SRC})
		target_compile_features(taskete_test-runner_THSAN PRIVATE cxx_std_17)
		target_include_directories(taskete_test-runner_THSAN PRIVATE test)
		target_compile_options(taskete_test-runner_THSAN PRIVATE ${CLANG_COMPILER_FLAGS} -fsanitize=thread)
		target_link_libraries(taskete_test-runner_THSAN PRIVATE taskete -fsanitize=thread)
		######

		add_test(NAME RunBaseTest COMMAND taskete_test-runner)
		add_test(NAME RunUBSAN COMMAND taskete_test-runner_UBSAN)
		add_test(NAME RunASAN COMMAND taskete_test-runner_ASAN)
		add_test(NAME RunTHSAN COMMAND taskete_test-runner_THSAN)

	else() # MSVC

		message("Configuring tests for MSVC...")

		add_executable(taskete_test-runner ${TEST_SRC})
		target_include_directories(taskete_test-runner PRIVATE test)
		target_compile_features(taskete_test-runner PRIVATE cxx_std_17)
		target_compile_definitions(taskete_test-runner PRIVATE _CRT_SECURE_NO_WARNINGS)

		target_link_libraries(taskete_test-runner PRIVATE taskete)

		add_test(NAME RunAllTests COMMAND taskete_test-runner)
	endif()
endif()
