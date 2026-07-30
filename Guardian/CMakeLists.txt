cmake_minimum_required(VERSION 3.20)
project(nicechunk_guardian LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GUARDIAN_BUILD_SERVER "Build the uWebSockets Guardian server" ON)
option(GUARDIAN_FETCH_UWS "Fetch uWebSockets if UWS_SOURCE_DIR is not provided" ON)
set(UWS_SOURCE_DIR "" CACHE PATH "Path to an existing uWebSockets checkout")

find_package(OpenSSL REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
  $<$<CONFIG:Release>:-O3>
)

set(GUARDIAN_CORE_SOURCES
  src/aoi.cpp
  src/chunk.cpp
  src/config.cpp
  src/guardian.cpp
  src/logging.cpp
  src/metrics.cpp
  src/player.cpp
  src/protocol.cpp
  src/time.cpp
  src/tui.cpp
)

add_library(nicechunk_guardian_core STATIC ${GUARDIAN_CORE_SOURCES})
target_include_directories(nicechunk_guardian_core PUBLIC src)
target_link_libraries(nicechunk_guardian_core PUBLIC OpenSSL::Crypto)

if(GUARDIAN_BUILD_SERVER)
  find_package(ZLIB REQUIRED)
  if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    set(GUARDIAN_OPENSSL_LIBS OpenSSL::SSL OpenSSL::Crypto)
  else()
    set(GUARDIAN_OPENSSL_LIBS ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
  endif()

  include(FetchContent)
  if(NOT UWS_SOURCE_DIR)
    if(NOT GUARDIAN_FETCH_UWS)
      message(FATAL_ERROR "Set -DUWS_SOURCE_DIR=/path/to/uWebSockets or enable GUARDIAN_FETCH_UWS")
    endif()
    FetchContent_Declare(
      uwebsockets
      GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
      GIT_TAG master
      GIT_SUBMODULES uSockets
      GIT_SUBMODULES_RECURSE FALSE
    )
    FetchContent_MakeAvailable(uwebsockets)
    set(UWS_SOURCE_DIR "${uwebsockets_SOURCE_DIR}")
  endif()

  if(NOT EXISTS "${UWS_SOURCE_DIR}/src/App.h")
    message(FATAL_ERROR "UWS_SOURCE_DIR must point to a uWebSockets checkout containing src/App.h")
  endif()

  set(USOCKETS_DIR "${UWS_SOURCE_DIR}/uSockets")
  if(NOT EXISTS "${USOCKETS_DIR}/src/libusockets.h")
    message(FATAL_ERROR "uSockets submodule not found at ${USOCKETS_DIR}. Run git submodule update --init --recursive.")
  endif()

  set(USOCKETS_SOURCES
    "${USOCKETS_DIR}/src/bsd.c"
    "${USOCKETS_DIR}/src/context.c"
    "${USOCKETS_DIR}/src/loop.c"
    "${USOCKETS_DIR}/src/socket.c"
    "${USOCKETS_DIR}/src/eventing/epoll_kqueue.c"
    "${USOCKETS_DIR}/src/crypto/openssl.c"
    "${USOCKETS_DIR}/src/crypto/sni_tree.cpp"
  )
  add_library(usockets STATIC ${USOCKETS_SOURCES})
  target_compile_definitions(usockets PRIVATE LIBUS_USE_OPENSSL)
  target_include_directories(usockets PUBLIC "${USOCKETS_DIR}/src")
  target_link_libraries(usockets PUBLIC ${GUARDIAN_OPENSSL_LIBS} ZLIB::ZLIB)

  add_executable(nicechunk-guardian
    src/main.cpp
    src/app.cpp
    ${GUARDIAN_CORE_SOURCES}
  )
  target_include_directories(nicechunk-guardian PRIVATE
    src
    "${UWS_SOURCE_DIR}/src"
    "${USOCKETS_DIR}/src"
  )
  target_link_libraries(nicechunk-guardian PRIVATE usockets ${GUARDIAN_OPENSSL_LIBS} ZLIB::ZLIB)
endif()

enable_testing()

add_executable(range_test tests/range_test.cpp)
target_link_libraries(range_test PRIVATE nicechunk_guardian_core)
add_test(NAME range_test COMMAND range_test)

add_executable(aoi_test tests/aoi_test.cpp)
target_link_libraries(aoi_test PRIVATE nicechunk_guardian_core)
add_test(NAME aoi_test COMMAND aoi_test)

add_executable(protocol_test tests/protocol_test.cpp)
target_link_libraries(protocol_test PRIVATE nicechunk_guardian_core)
add_test(NAME protocol_test COMMAND protocol_test)

add_executable(building_manifest_test tests/building_manifest_test.cpp)
target_link_libraries(building_manifest_test PRIVATE nicechunk_guardian_core)
add_test(NAME building_manifest_test COMMAND building_manifest_test)

add_executable(load_test tests/load_test.cpp)
target_link_libraries(load_test PRIVATE nicechunk_guardian_core)
add_test(NAME load_test COMMAND load_test)
