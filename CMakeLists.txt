cmake_minimum_required(VERSION 3.10)
project(ColliderFramework)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

find_package(ROOT REQUIRED COMPONENTS Core Tree Hist RIO Physics Graf MathCore Gpad)
include(${ROOT_USE_FILE})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Geometry/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Math/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Transport/include
)

file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/Geometry/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/Math/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/Transport/src/*.cxx"
)

set(MAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/main.cxx)

ROOT_GENERATE_DICTIONARY(G__Event 
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/include/Event.h
    LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/Core/include/LinkDef.h
)

add_executable(collider ${MAIN_FILE} ${SOURCES} G__Event.cxx)

target_link_libraries(collider PUBLIC ROOT::Core ROOT::Tree ROOT::Hist ROOT::RIO ROOT::Physics ROOT::Graf ROOT::MathCore ROOT::Gpad)

set_target_properties(collider PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

add_custom_command(TARGET collider POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_BINARY_DIR}/libEvent_rdict.pcm"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)