message(STATUS "Building VoltDB large record size reproduction src")

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(_VOLTDB_LIBS_PATH "${CMAKE_SOURCE_DIR}/third_party/voltdb-client-cpp/lib")

include_directories("${CMAKE_SOURCE_DIR}/third_party")
include_directories(/usr/local/include)
link_directories(/usr/local/lib /usr/local/lib64)

# Dependencies
# Find boost static libs, statically linked to C++ runtime.
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

find_package(Boost 1.53 COMPONENTS system thread)
message(STATUS "Using Boost ${Boost_VERSION}")

set(lib_util_SOURCES BenchmarkUtil.cc)
add_library(lib_util STATIC ${lib_util_SOURCES})

target_include_directories(lib_util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(reproduction reproduction.cc)

target_link_libraries(reproduction
                      lib_util
                      ${_VOLTDB_LIBS_PATH}/libvoltdbcpp.a
                      ${_VOLTDB_LIBS_PATH}/libevent.a
                      ${_VOLTDB_LIBS_PATH}/libevent_openssl.a
                      ${_VOLTDB_LIBS_PATH}/libevent_pthreads.a
                      ${_VOLTDB_LIBS_PATH}/libssl.a
                      ${_VOLTDB_LIBS_PATH}/libcrypto.a
                      pthread Boost::system Boost::thread dl)

set_target_properties(lib_util reproduction
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
