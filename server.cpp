#include <vector>
#include <sstream>
#include <fstream>
#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <filesystem>
#include "header.hpp"

Request::Get deserialize_get(std::string& str)
{
  std::vector<std::string> tokens;
  std::stringstream ss(str);
  std::string token;
  std::string buffer;
  while(std::getline(ss, token, '|'))
  {
    int i = 0;
    while(std::isdigit(token.at(i)))
    {
      buffer.push_back(token.at(i));
      i++; 
    }
    std::cout << "size: " << buffer << std::endl;
    size_t size = std::stoi(buffer);
    buffer.clear();
    std::cout << "from: " << i + 1 << " ,to: " << i + size << std::endl; 
    tokens.push_back(token.substr(i + 1, i + size));
  }
  Request::Get req = {
    .path = tokens[1],
    .et = (EntryType)std::stoi(tokens[2])
  };
  return req;
}

std::optional<std::string> read_folder(const std::string& path)
{
  try
  {
    if (!std::filesystem::is_directory(path))
    {
      std::cerr << path << " is not a directory." << std::endl;
      return std::nullopt;
    }
    std::stringstream ss;
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
      if (entry.is_directory())
        ss << "📁 " << entry.path().filename().string();
      else if (entry.is_regular_file())
        ss << "📄 " << entry.path().filename().string();
    }
    return ss.str();
  }
  catch (const std::filesystem::filesystem_error& e)
  {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
    return std::nullopt;
  }
}

void handle_send(SOCKET clientSocket, std::string& response);

std::optional<std::string> read_file(const std::string& path)
{
  std::ifstream file(path);
  if(!file.is_open())
  {
    std::cerr << "file " << path << " could not be opened.." << std::endl;
    return std::nullopt;
  }
  std::stringstream ss;
  std::string line;
  while(std::getline(file, line))
    ss << line;
  return ss.str();
}

std::string serialize_response(std::string& content)
{
  return content;
}

void receive(SOCKET acceptSocket)
{
  char buffer[200];
  memset(buffer, 0, sizeof(buffer));
  int byteCount = recv(acceptSocket, buffer, 200, 0);
  if(byteCount > 0)
    std::cout << "Received message: \"" << buffer << "\"" << std::endl;
  else
  {
    std::cout << "Didn't receive anything." << std::endl;
    WSACleanup();
  }
  std::string str(buffer);
  Request::Get req = deserialize_get(str);
  std::cout << "request path: " << req.path << "\n" << "request et: " << (int)req.et << std::endl;
  std::string path = BASE_PATH + req.path;
  auto content = read_file(path);
  if(!content.has_value())
  {
    std::cerr << "Couldn't read file: " << path << std::endl;
  }
  else
  {
    std::string response = serialize_response(content.value());
    handle_send(acceptSocket, response);
  }
}

void handle_send(SOCKET clientSocket, std::string& response) {
  int byteCount = send(clientSocket, response.c_str(), 200, 0);
  if(byteCount > 0) {
    std::cout << "Sent message: \"" << response << "\"" << std::endl;
  } else {
    std::cout << "Nothing was sent." << std::endl;
    WSACleanup();
  }
}

int main()
{
  SOCKET serverSocket, acceptSocket;
  const int PORT = 55555;
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2, 2);
  int wsaerr = WSAStartup(wVersionRequested, &wsaData);

  if (wsaerr != 0) {
    std::cerr << "Winsock dll not found." << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cout << "Winsock dll was found!" << std::endl;
    std::cout << "Status: " << wsaData.szSystemStatus << std::endl;
  }

  serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (serverSocket == INVALID_SOCKET) {
    std::cerr << "Error at socket()." << std::endl;
    WSACleanup();
    return EXIT_FAILURE;
  } else {
    std::cout << "Socket() is OK!" << std::endl;
  }

  sockaddr_in service;
  service.sin_family = AF_INET;
  InetPton(AF_INET, _T("127.0.0.1"), &service.sin_addr.s_addr);
  service.sin_port = htons(PORT);

  if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
    std::cerr << "bind() failed." << std::endl;
    std::cerr << WSAGetLastError() << std::endl;
    closesocket(serverSocket);
    WSACleanup();
    return EXIT_FAILURE;
  } else {
    std::cout << "bind() is OK!" << std::endl;
  }

  if (listen(serverSocket, 2) == SOCKET_ERROR) {
    std::cerr << "listen() error listening on socket." << std::endl;
    closesocket(serverSocket);
    WSACleanup();
    return EXIT_FAILURE;
  } else {
    std::cout << "listen() is OK!, waiting for connections.." << std::endl;
  }

  while(true)
  {
    acceptSocket = accept(serverSocket, NULL, NULL);
    if (acceptSocket == INVALID_SOCKET) {
      std::cerr << "accept failed." << std::endl;
      std::cerr << WSAGetLastError() << std::endl;
      closesocket(serverSocket);
      WSACleanup();
      return EXIT_FAILURE;
    }
    std::cout << "Client connected.." << std::endl;
    std::thread thread(receive, acceptSocket);
    thread.detach();
  }
  close_socket(serverSocket);
  WSACleanup();
}