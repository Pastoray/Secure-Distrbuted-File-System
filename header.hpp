#pragma once

#include <iostream>
#include <optional>

#define BASE_PATH "C:\\Users\\pc\\Desktop\\folder\\"

enum class RequestType
{
  GET,
  CREATE,
  UPDATE,
  REMOVE
};

enum class EntryType
{
  File,
  Folder
};

enum class UpdateType
{
  Override,
  Append
};

namespace Request
{
  struct Get {
    std::string path;
    EntryType et;
  };
  struct Create {
    std::string path;
    EntryType et;
    std::string name;
    std::optional<std::string> content; // if file
  };
  struct Update
  {
    std::string path;
    EntryType et;
    UpdateType ut;
  };
  struct Remove {
    std::string path;
    EntryType et;
  };
}

void close_socket(SOCKET socket)
{
  if (shutdown(socket, SD_BOTH) == SOCKET_ERROR) {
    std::cerr << "Shutdown failed with error: " << WSAGetLastError() << std::endl;
  }
  if (closesocket(socket) == SOCKET_ERROR) {
    std::cerr << "Close socket failed with error: " << WSAGetLastError() << std::endl;
  }
}