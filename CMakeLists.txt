cmake_minimum_required(VERSION 3.10)
project(file_sharing C)

set(CMAKE_C_STANDARD 11)

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)

# Add include directories
include_directories(${JSONC_INCLUDE_DIRS})

# Link libraries
link_libraries(${JSONC_LIBRARIES} mysqlclient)

# Create executables
add_executable(server src/server.c src/db_utils.c src/models/join_model.c src/models/group_model.c src/models/user_model.c src/models/folder_model.c src/controllers/file_controller.c src/db/db_access.c src/server/file_handler.c)
add_executable(client src/client.c)