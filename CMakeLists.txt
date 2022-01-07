CMAKE_MINIMUM_REQUIRED(VERSION 2.8 FATAL_ERROR)

PROJECT(spr_client)
set(CMAKE_CXX_STANDARD 14) 
find_package(PkgConfig REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread)
set(SRC_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/spr_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/browseZeroConf/browse_bonjour.cpp
)

set(SERVER_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/browseZeroConf
)

INCLUDE_DIRECTORIES(${SERVER_INCLUDE})
#pkg_check_modules(PROCESS_CPP process-cpp)

ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_LIST})
target_compile_options(${PROJECT_NAME} PUBLIC -O -g -std=c++11)

target_link_libraries(
    ${PROJECT_NAME}
    pthread
    dns_sd
    ${ADK_MESSAGE_SERVICE_LDFLAGS}
    ${PROCESS_CPP_LIBRARIES}
    ${ADK_IPC_LIBRARIES}
    ${ADK_CONFIG_LIBRARIES}
    ${DBUSCPP_LIBRARIES}
    ${ADK_MESSAGE_SERVICE_LIBRARIES}
    ${Boost_LIBRARIES}
)

include_directories(${Boost_INCLUDE_DIRS})

target_include_directories(
  ${PROJECT_NAME} PUBLIC
  ${ADK_MESSAGE_SERVICE_INCLUDE_DIRS}
  ${ADK_IPC_INCLUDE_DIRS}
  ${ADK_CONFIG_INCLUDE_DIRS}
  ${DBUSCPP_INCLUDE_DIRS}
)

install (TARGETS spr_client COMPONENT snap DESTINATION bin)
