set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.7)
project(C_Controller)

if(CMAKE_HOST_WIN32)
    set(TARGET native)
    set(EXECUTABLE simulateur.exe)
elseif(NOT TARGET)
    set(TARGET stm32f303)
    set(EXECUTABLE ${PROJECT_NAME}.${TARGET}.elf)
else()
    set(EXECUTABLE simulateur)
endif()

set(LOWLEVEL_INC_DIR ${CMAKE_SOURCE_DIR}/lowlevel/include)
set(LOWLEVEL_SRC_DIR ${CMAKE_SOURCE_DIR}/lowlevel/${TARGET})
set(SOURCES
    platform.h
    main.c
    controller.h
    controller.c
    configuration.h
    configuration.c
    alarms.h
    alarms.c
    ihm_communication.h
    ihm_communication.c
    ${LOWLEVEL_INC_DIR}/hardware_simulation.h
    ${LOWLEVEL_SRC_DIR}/hardware_simulation.c
    unit_tests.h
    unit_tests.c)

add_executable(${EXECUTABLE} ${SOURCES})

include(${CMAKE_SOURCE_DIR}/lowlevel/${TARGET}/${TARGET}.cmake)

include_directories(${CMAKE_SOURCE_DIR} ${LOWLEVEL_INC_DIR})
target_link_libraries(${EXECUTABLE} m)
set_target_properties(${EXECUTABLE} PROPERTIES LINKER_LANGUAGE C)
