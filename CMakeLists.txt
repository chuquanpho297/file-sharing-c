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
    src/server/db/db_access.c
    src/server/system/system_access.c
    src/server/server.c
    src/server/handler/file_handler.c
    src/server/handler/folder_handler.c
    src/server/handler/user_handler.c
    src/utils/helper.c
    src/utils/structs.c
)
add_executable(client src/client.c src/utils/client_utils.c)

# Add include directories
include_directories(${JSONC_INCLUDE_DIRS})

# Update link libraries
target_link_libraries(server ${JSONC_LIBRARIES} mysqlclient pthread)
target_link_libraries(client ${JSONC_LIBRARIES} pthread)
