CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

PROJECT(at_cmd_handler)

SET(ROOT_DIR "${CMAKE_SOURCE_DIR}")
SET(PRJ_NAME "${CMAKE_PROJECT_NAME}")

# Generate compilation database
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Put the binary file into bin directory
SET(EXECUTABLE_OUTPUT_PATH "${ROOT_DIR}/bin")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-4 -g3")
ENDIF()

SET(EXT_DEPS ${ROOT_DIR}/external_dependencies)
SET(UNITY ${EXT_DEPS}/unity/src)
SET(FREERTOS ${EXT_DEPS}/FreeRTOS)
SET(UNIT_TESTS_NON_RTOS_DIR ${ROOT_DIR}/tests/non-rtos)
SET(UNIT_TESTS_RTOS_DIR ${ROOT_DIR}/tests/rtos)
SET(SRC_DIR ${ROOT_DIR}/src)

IF(UNIT_TEST_NON_RTOS)
    SET(BIN_SUFFIX "local_test")

    INCLUDE_DIRECTORIES(
        ${UNITY}
        ${SRC_DIR}
        ${ROOT_DIR}
        ${UNIT_TESTS_NON_RTOS_DIR}
        ${ROOT_DIR}/tests
        )

    FILE(GLOB SOURCES
        "${UNIT_TESTS_NON_RTOS_DIR}/*.c*"
        )

    ADD_EXECUTABLE(${PRJ_NAME}
        ${SOURCES}
        ${SRC_DIR}/at_cmd_handler.cpp
        ${SRC_DIR}/string_buf_tx.cpp
        ${UNITY}/unity.c
        )

ELSEIF(UNIT_TEST_RTOS)
    SET(BIN_SUFFIX "rtos_local_test")

    SET(FREERTOS_PORT ${FREERTOS}/portable/GCC/Linux)
    SET(FREERTOS_SOURCES ${FREERTOS}/queue.c ${FREERTOS}/list.c ${FREERTOS}/tasks.c ${FREERTOS}/timers.c
        ${FREERTOS_PORT}/port.c ${FREERTOS}/portable/MemMang/heap_3.c)

    SET(THREADS_PREFER_PTHREAD_FLAG ON)
    FIND_PACKAGE(Threads REQUIRED)

    INCLUDE_DIRECTORIES(
        ${FREERTOS_PORT}
        ${FREERTOS}/include
        ${FREERTOS}/include/private
        ${UNITY}
        ${SRC_DIR}
        ${ROOT_DIR}
        ${UNIT_TESTS_RTOS_DIR}
        ${ROOT_DIR}/tests
        ${EXT_DEPS}
        )

    FILE(GLOB SOURCES
        "${UNIT_TESTS_RTOS_DIR}/*.c*"
        "${SRC_DIR}/*.c*"
        )

    ADD_EXECUTABLE(${PRJ_NAME}
        ${FREERTOS_SOURCES}
        ${SOURCES}
        ${UNITY}/unity.c
        )

    TARGET_LINK_LIBRARIES(${PRJ_NAME} Threads::Threads)
ELSE()
ENDIF()

# Add "tags" target and make the project depending on this target.
SET_SOURCE_FILES_PROPERTIES(tags PROPERTIES GENERATED true)
ADD_CUSTOM_TARGET(tags
    COMMAND ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .
    WORKING_DIRECTORY ${ROOT_DIR})
ADD_DEPENDENCIES(${PRJ_NAME} tags)

SET_TARGET_PROPERTIES(${PRJ_NAME} PROPERTIES OUTPUT_NAME "${PRJ_NAME}-${BIN_SUFFIX}")

ADD_CUSTOM_TARGET(run-test
    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ${ROOT_DIR}/bin/${PRJ_NAME}-${BIN_SUFFIX}
    )

