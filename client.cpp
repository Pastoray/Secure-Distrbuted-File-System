#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "utils.hpp"

void get(SOCKET acceptSocket) {
  char buffer[200];
  while(true) {
    memset(buffer, 0, sizeof(buffer));
    int byteCount = recv(acceptSocket, buffer, 200, 0);
    if(byteCount > 0) {
      std::cout << "Received message: \"" << buffer << "\"" << std::endl;
    } else {
      std::cout << "Didn't receive anything." << std::endl;
      WSACleanup();
    }
  }
}

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
  char buffer[200];
  memset(buffer, 0, sizeof(buffer));
  int byteCount = recv(acceptSocket, buffer, 200, 0);
  if(byteCount > 0) {
    std::cout << "Received message: \"" << buffer << "\"" << std::endl;
  } else {
    std::cerr << "Didn't receive anything." << std::endl;
    WSACleanup();
  }
  std::string str(buffer);
  Res res = Transformer::Deserialize::response(str);
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
    return 0;
  } else {
    std::cout << "Winsock dll was found!" << std::endl;
    std::cout << "Status: " << wsaData.szSystemStatus << std::endl;
  }

  clientSocket = INVALID_SOCKET;
  clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if(clientSocket == INVALID_SOCKET) {
    std::cout << "Error at socket()." << std::endl;
    return 0;
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

void get_request(std::string& path, std::string& entity)
{
  Request::Get req = { .path = path };
  if(entity == "file")
    req.et = EntryType::FILE;
  else if(entity == "folder")
    req.et = EntryType::DIR;
  SOCKET socket = create_socket();
  std::string str = Transformer::Serialize::request(req);
  handle_send(socket, str);
  handle_recv(socket);
  close_socket(socket);
};

void create_request(std::string& path, std::string& entity)
{

};

void update_request(std::string& path, std::string& entity)
{

};

void remove_request(std::string& path, std::string& entity)
{

};

int main(int argc, char* argv[])
{
  if(argc != 4)
  {
    std::cerr << "Wrong usage,\n"
      << "Correct usage: ./sdfs <request> <\"file\"/\"folder\"> <path>\n"
      << std::endl;
    return EXIT_FAILURE;
  }
  std::string request = argv[1], entity = argv[2], path = argv[3];
  if(entity != "file" && entity != "folder")
  {
    std::cerr << "Unrecognized entity: " << entity << std::endl;
    return EXIT_FAILURE;
  }

  if(request == MessageType::GET)
    get_request(path, entity);
  else if(request == MessageType::CREATE)
    create_request(path, entity);
  else if(request == MessageType::EDIT)
    update_request(path, entity);
  else if(request == MessageType::REMOVE)
    remove_request(path, entity);
  else
  {
    std::cerr << "Unrecognized request type: " << request << std::endl ;
    return EXIT_FAILURE;
  }
  WSACleanup();
}