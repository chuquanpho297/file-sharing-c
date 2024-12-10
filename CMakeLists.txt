cmake_minimum_required(VERSION 3.10)
project(file_sharing C)

set(CMAKE_C_STANDARD 11)

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)

# Create executables
add_executable(server src/server.c)
add_executable(client src/client.c)

# Link libraries
target_link_libraries(server PRIVATE
    Threads::Threads
    json-c
)
target_link_libraries(client PRIVATE
    json-c
)
