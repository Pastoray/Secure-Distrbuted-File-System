# SDFS (Secure Distributed File System)

## Overview

SDFS is a secure and distributed file system designed for efficient cross-network file management with built-in encryption. It provides a robust solution for storing and retrieving files across multiple nodes while ensuring data confidentiality and integrity through encryption mechanisms.

## Features

- **Security**: All files are encrypted before being stored, ensuring data security.
- **Filesystem**: Files are organized and managed on a single storage device, ensuring data integrity and accessibility.
- **Cross-Network Operation**: Supports communication and file operations across different networks.

## Getting Started

### Prerequisites

- **Windows OS/VM**: As of right now, this app uses the winsock API, therefore only usable on windows.
- **C++ Compiler**: Ensure you have a C++ compiler installed (e.g., MinGW).
- **CMake**: Required for building the project. [Download CMake](https://cmake.org/download/) if you haven't already.
- **Libraries**: Ensure the required libraries (`ws2_32`, `ssl`, `crypto`) are installed.

### Building the Project

1. Clone the repository:

   ```
   git clone <repository-url>
   cd <repository-directory>
   ```

2. Create a build directory:

   ```
   mkdir out/build
   ```

3. Configure the project with CMake:

   ```
   cmake -S . -B out/build/ -G "MinGW Makefiles"
   ```

4. Build the project:

   ```
   cd out/build
   mingw32-make
   ```

### Running the Application

After building, you will have two executables: `server.exe` and `client.exe`.

- **Server**: Start the server to handle file operations and requests as well as manage distributed storage.

  ```
  ./server
  ```

- **Client**: Use the client application to interact with the server, get, upload, edit and remove files and folders. Examples:

  ```
  ./client GET --entry DIR --path .
  ./client CREATE --entry FILE --path . --name "new File" --content "Hello world"
  ./client EDIT --entry FILE --path . --name "new File" --content "Hello world" --edit-type OVERRIDE
  ./client REMOVE --entry DIR --path .
  ```

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request if you have suggestions or improvements.
