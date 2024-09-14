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
#include "transformer.hpp"
#include "crypto.hpp"

void handle_send(SOCKET clientSocket, std::string& req)
{
  std::string encryptedStr = Crypto::encrypt(req);
  int byteCount = send(clientSocket, encryptedStr.c_str(), encryptedStr.size(), 0);
  if(byteCount > 0)
    std::cout << "Sent message: \"" << req << "\"" << std::endl;
  else
  {
    std::cout << "Nothing was sent." << std::endl;
    WSACleanup();
  }
}

void handle_recv(SOCKET acceptSocket)
{
  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  int byteCount = recv(acceptSocket, buffer, sizeof(buffer), 0);
  std::string decryptedRes;
  if(byteCount > 0)
  {
    decryptedRes = Crypto::decrypt(buffer);
    std::cout << "Received message: \"" << decryptedRes << "\"" << std::endl;
  }
  else
  {
    std::cerr << "Didn't receive anything." << std::endl;
    WSACleanup();
  }
  std::string str(decryptedRes);
  Res res = Transformer::Respons::deserialize(str);
  SetConsoleOutputCP(CP_UTF8); // to be able to cout emojis
  Transformer::Respons::print(res);
}

SOCKET create_socket()
{
  SOCKET clientSocket;
  const int PORT = 55555;
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2, 2);
  int wsaerr = WSAStartup(wVersionRequested, &wsaData);

  if(wsaerr != 0)
  {
    std::cout << "Winsock dll not found" << std::endl;
    err_exit("Winsock dll not found");
  }
  else
  {
    std::cout << "Winsock dll was found" << std::endl;
    std::cout << "Status: " << wsaData.szSystemStatus << std::endl;
  }

  clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(clientSocket == INVALID_SOCKET)
  {
    std::cout << "Error at socket()" << std::endl;
    WSACleanup();
    return EXIT_FAILURE;
  }
  else
    std::cout << "Socket() is OK" << std::endl;

  sockaddr_in clientService;
  clientService.sin_family = AF_INET;
  InetPton(AF_INET, _T("127.0.0.1"), &clientService.sin_addr.s_addr);
  clientService.sin_port = htons(PORT);

  if(connect(clientSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR)
  {
    std::cout << "Client connect() failed" << std::endl;
    WSACleanup();
    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "Client connect() is OK" << std::endl;
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
  if(argc == 2)
  {
    if(argv[1] == "?")
    {
      std::cout << "Usage: \n"
        << "GET [FILE/DIR] <path>\n"
        << "CREATE [FILE/DIR] <path> <name> <content (optional; only if file)>\n"
        << "EDIT [FILE/DIR] <path> <name (optional)> <content (optional)> [OVERRIDE/APPEND]\n"
        << "REMOVE [FILE/DIR] <path>" << std::endl;
      return EXIT_SUCCESS;
    }
  }
  if(argc <= 2)
  {
    std::cerr << "Wrong usage,\nCorrect usage: ./client <request> <args...>\n"
      << "Use \"./client ?\" for more details" << std::endl;
    return EXIT_FAILURE;
  }

  /*
    argv[1] = request type
    argv[2] = entry type
    argv[3] = path
    argv[4] = name (create, edit)
    argv[5] = content (create, edit)
    argv[6] = edit type (edit)
  */

  if(argv[2] != "FILE" && argv[2] != "DIR")
  {
    std::cerr << "Invalid entity: " << argv[2] << std::endl;
    return EXIT_FAILURE;
  }
  EntryType et = argv[2] == "FILE" ? EntryType::FILE : EntryType::DIR;
  std::unordered_map<std::string, std::function<Req()>> request_map = {
    {MessageType::GET, [&]() -> Request::Get {
      auto req =  create_req<Request::Get>(et, argv[3]);
      return req;
    }},
    {MessageType::CREATE, [&]() -> Request::Create {
      auto req =  create_req<Request::Create>(et, argv[3], argv[4], argv[5]);
      return req;
    }},
    {MessageType::EDIT, [&]() -> Request::Edit {
      auto req =  create_req<Request::Edit>(et, argv[3], argv[4], argv[5], (EditType)std::stoi(argv[6]));
      return req;
    }},
    {MessageType::REMOVE, [&]() -> Request::Remove {
      auto req =  create_req<Request::Remove>(et, argv[3]);
      return req;
    }}
  };
  Req request;
  if (request_map.find(argv[1]) != request_map.end())
    request = request_map[argv[1]]();
  else
    err_exit("Unknown request type");
  
  SOCKET socket = create_socket();
  std::string str = Transformer::Reques::serialize(request);
  handle_send(socket, str);
  handle_recv(socket);
  close_socket(socket);
  WSACleanup();
}