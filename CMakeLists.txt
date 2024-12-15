cmake_minimum_required(VERSION 3.10)
project(file_sharing C)

set(CMAKE_C_STANDARD 11)

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)

# Link libraries
link_libraries(${JSONC_LIBRARIES} mysqlclient)

# Create executables
add_executable(server
    src/server/server.c
    src/server/db/db_access.c
    src/server/controllers/file_controller.c
    src/server/file_handler.c
    src/server/group_handler.c
    src/server/models/folder_model.c
    src/server/models/group_model.c
    src/server/models/join_model.c
    src/server/models/user_model.c
)
add_executable(client src/client.c)
