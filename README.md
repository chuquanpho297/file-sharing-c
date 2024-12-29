# Project Setup and Build Instructions

## Prerequisites

Before you begin, ensure you have the following installed on your system:

### On Ubuntu
- CMake (version 3.22 or higher)
- Mysql 8.0
- Libraries: `json-c`, `libzip`, `mysqlclient`, and `clang-format`

  ```bash
  sudo apt-get update
  sudo apt-get install -y build-essential cmake libjson-c-dev libzip-dev libmysqlclient-dev clang-format
  ```
## Building the Project

1. **Configure the Project**

   Use CMake to configure the project. This will generate the necessary build files:

   ```bash
   cp .env.example .env
   mkdir build
   cd build
   cmake ..
   ```

2. **Build the Project**

   Compile the project using the generated build files:

   ```bash
   make
   ```

   This will create the executables for the server, client, and test programs.

## Running the Executables

- **Server**: Run the server executable:

  ```bash
  ./server
  ```

- **Client**: Run the client executable:

  ```bash
  ./client
  ```

## Formatting the Code

To format all source files using `clang-format`, run the following command:

```bash
make format
```

This will automatically format all `.c` and `.h` files in the project.
