#include <vector>
#include <sstream>
#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <windows.h>
#include "utils.hpp"
#include "transformer.hpp"
#include "crypto.hpp"

void handle_send(SOCKET clientSocket, std::string& res)
{
  std::string encryptedRes = Crypto::encrypt(res);
  int byteCount = send(clientSocket, encryptedRes.c_str(), encryptedRes.size(), 0);
  if(byteCount > 0)
    std::cout << "Sent message: \"" << res << "\"" << std::endl;
  else
  {
    std::cout << "Nothing was sent." << std::endl;
    WSACleanup();
  }
}

void handle_receive(SOCKET acceptSocket)
{
  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  int byteCount = recv(acceptSocket, buffer, sizeof(buffer), 0);
  std::string decryptedReq;
  if(byteCount > 0)
  {
    decryptedReq = Crypto::decrypt(std::string(buffer, byteCount));
    std::cout << "Received message: \"" << decryptedReq << "\"" << std::endl;
  }
  else
  {
    std::cout << "Didn't receive anything." << std::endl;
    WSACleanup();
  }
  auto req = Transformer::Reques::deserialize(decryptedReq);
  auto res = process_request(req);
  std::string response = Transformer::Respons::serialize(res);
  handle_send(acceptSocket, response);
}

int main(int argc, char* argv[])
{
  // if(argc != 3 || std::strcmp(argv[1], "--key") != 0 || std::strlen(argv[2]) != 64)
  // {
  //   std::cerr << "Wrong usage,\nCorrect usage: ./server --key <32-byte-key>" << std::endl;
  //   return EXIT_FAILURE;
  // }
  
  SOCKET serverSocket, acceptSocket;
  const int PORT = 55555;
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2, 2);
  int wsaerr = WSAStartup(wVersionRequested, &wsaData);

  if (wsaerr != 0)
  {
    std::cerr << "Winsock dll not found" << std::endl;
    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "Winsock dll was found" << std::endl;
    std::cout << "Status: " << wsaData.szSystemStatus << std::endl;
  }

  serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (serverSocket == INVALID_SOCKET)
  {
    std::cerr << "Error at socket()" << std::endl;
    WSACleanup();
    return EXIT_FAILURE;
  }
  else
    std::cout << "Socket() is OK" << std::endl;

  sockaddr_in service;
  service.sin_family = AF_INET;
  InetPton(AF_INET, _T("127.0.0.1"), &service.sin_addr.s_addr);
  service.sin_port = htons(PORT);

  if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
  {
    std::cerr << "bind() failed" << std::endl;
    std::cerr << WSAGetLastError() << std::endl;
    closesocket(serverSocket);
    WSACleanup();
    return EXIT_FAILURE;
  }
  else
    std::cout << "bind() is OK!" << std::endl;

  if (listen(serverSocket, 2) == SOCKET_ERROR)
  {
    std::cerr << "listen() error listening on socket" << std::endl;
    closesocket(serverSocket);
    WSACleanup();
    return EXIT_FAILURE;
  }
  else
    std::cout << "listen() is OK!, waiting for connections.." << std::endl;

  while(true)
  {
    acceptSocket = accept(serverSocket, NULL, NULL);
    if (acceptSocket == INVALID_SOCKET)
    {
      std::cerr << "accept failed" << std::endl;
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