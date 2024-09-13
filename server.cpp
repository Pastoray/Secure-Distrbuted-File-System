#include <vector>
#include <sstream>
#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <windows.h>
#include "utils.hpp"

void handle_send(SOCKET clientSocket, std::string& response) {
  int byteCount = send(clientSocket, response.c_str(), 200, 0);
  if(byteCount > 0) {
    std::cout << "Sent message: \"" << response << "\"" << std::endl;
  } else {
    std::cout << "Nothing was sent." << std::endl;
    WSACleanup();
  }
}

void handle_receive(SOCKET acceptSocket)
{
  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  int byteCount = recv(acceptSocket, buffer, 200, 0);
  if(byteCount > 0)
    std::cout << "Received message: \"" << buffer << "\"" << std::endl;
  else
  {
    std::cout << "Didn't receive anything." << std::endl;
    WSACleanup();
  }
  std::string encryptedStr(buffer);
  std::string str = Transformer::Reques::decrypt(encryptedStr);
  auto req = Transformer::Reques::deserialize(str);
  auto res = process_request(req);
  std::string response = Transformer::Respons::serialize(res);
  std::string encryptedResponse = Transformer::Respons::encrypt(response);
  handle_send(acceptSocket, encryptedResponse);
}

int main(int argc, char* argv[])
{
  if(argc != 3 || std::strcmp(argv[1], "--key") != 0 || std::strlen(argv[2]) != 64)
  {
    std::cerr << "Wrong usage,\nCorrect usage: ./server --key <32-bytes-key>" << std::endl;
    return EXIT_FAILURE;
  }
  Transformer::set_key(argv[2]);
  
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
    std::thread thread(handle_receive, acceptSocket);
    thread.detach();
  }
  close_socket(serverSocket);
  WSACleanup();
}