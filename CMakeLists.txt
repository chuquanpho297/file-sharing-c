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
    src/server/models/user_model.c
    src/server/db/db_access.c
    src/server/controllers/file_controller.c
    src/server/controllers/user_controller.c
    src/server/file_handler.c
    src/server/server.c
)
add_executable(client src/client.c)
