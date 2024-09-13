#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <unordered_map>
#include <functional>
#include "utils.hpp"

void handle_send(SOCKET clientSocket, std::string& req) {
  int byteCount = send(clientSocket, req.c_str(), 200, 0);
  if(byteCount > 0) {
    std::cout << "Sent message: \"" << req << "\"" << std::endl;
  } else {
    std::cout << "Nothing was sent." << std::endl;
    WSACleanup();
    return;
  }
}

void handle_recv(SOCKET acceptSocket) {
  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  int byteCount = recv(acceptSocket, buffer, 200, 0);
  if(byteCount > 0) {
    std::cout << "Received message: \"" << buffer << "\"" << std::endl;
  } else {
    std::cerr << "Didn't receive anything." << std::endl;
    WSACleanup();
  }
  std::string str(buffer);
  Res res = Transformer::Respons::deserialize(str);
  SetConsoleOutputCP(CP_UTF8); // to be able to cout emojis
  print_res(res);
}

SOCKET create_socket()
{
  SOCKET clientSocket;
  const int PORT = 55555;
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2, 2);
  int wsaerr = WSAStartup(wVersionRequested, &wsaData);

  if(wsaerr != 0) {
    std::cout << "Winsock dll not found." << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cout << "Winsock dll was found!" << std::endl;
    std::cout << "Status: " << wsaData.szSystemStatus << std::endl;
  }

  clientSocket = INVALID_SOCKET;
  clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if(clientSocket == INVALID_SOCKET) {
    std::cout << "Error at socket()." << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cout << "Socket() is OK!" << std::endl;
  }

  sockaddr_in clientService;
  clientService.sin_family = AF_INET;
  InetPton(AF_INET, _T("127.0.0.1"), &clientService.sin_addr.s_addr);
  clientService.sin_port = htons(PORT);

  if(connect(clientSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
    std::cout << "Client connect() failed." << std::endl;
    WSACleanup();
    return 0;
  } else {
    std::cout << "Client connect() is OK!" << std::endl;
    std::cout << "Client can start sending and receiving data" << std::endl;    
  }

  return clientSocket;
}

template<typename T, typename ...Args>
T create_req(Args&&... args)
{
  return T { std::forward<Args>(args)... };
}

int main(int argc, char* argv[])
{
  if(argc <= 2)
  {
    std::cerr << "Wrong usage,\nCorrect usage: ./client <request> <args...>" << std::endl;
    return EXIT_FAILURE;
  }
  
  std::string request = argv[1], entity = argv[2], path = argv[3];
  if(entity != "file" && entity != "dir")
  {
    std::cerr << "Unrecognized entity: " << entity << std::endl;
    return EXIT_FAILURE;
  }
  EntryType et = (entity == "file" ? EntryType::FILE : EntryType::DIR);
  std::unordered_map<std::string, std::function<Req()>> request_map = {
    {MessageType::GET, [&]() -> Request::Get {
      auto req =  create_req<Request::Get>(path, et);
      return req;
    }},
    {MessageType::CREATE, [&]() -> Request::Create {
      auto req =  create_req<Request::Create>(path, et, argv[4], argv[5]);
      return req;
    }},
    {MessageType::EDIT, [&]() -> Request::Edit {
      auto req =  create_req<Request::Edit>(path, et, argv[4], argv[5], (UpdateType)std::stoi(argv[6]));
      return req;
    }},
    {MessageType::REMOVE, [&]() -> Request::Remove {
      auto req =  create_req<Request::Remove>(path, et);
      return req;
    }}
  };
  Req req;
  if (request_map.find(request) != request_map.end())
    req = request_map[request]();
  else
    err_exit("Unknown request type");
  
  SOCKET socket = create_socket();
  std::string str = Transformer::Reques::serialize(req);
  std::string encryptedStr = Transformer::Reques::encrypt(str);
  handle_send(socket, encryptedStr);
  handle_recv(socket);
  close_socket(socket);
  WSACleanup();
}