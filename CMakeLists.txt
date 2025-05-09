cmake_minimum_required(VERSION 3.10)
project(gvfs-limit C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig)

pkg_check_modules(GIO REQUIRED gio-2.0)

add_library(gvfs-limit SHARED src/vfs-limit.c src/vfs-limit.h)
target_link_libraries(gvfs-limit PUBLIC ${GIO_LIBRARIES})
target_include_directories(gvfs-limit PUBLIC ${GIO_INCLUDE_DIRS})

add_executable(file-new example/file-new.c)
target_link_libraries(file-new PUBLIC ${GIO_LIBRARIES} gvfs-limit)
target_compile_options(file-new PUBLIC -Wl,rpath=${CMAKE_BINARY_DIR}/)
target_include_directories(file-new PUBLIC ${GIO_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src)
