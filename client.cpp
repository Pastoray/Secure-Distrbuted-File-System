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
#include "include/utils.hpp"
#include "include/transformer.hpp"
#include "include/crypto.hpp"

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
  Res res = Transformer::TResponse::deserialize(str);
  SetConsoleOutputCP(CP_UTF8); // to be able to cout emojis
  Transformer::TResponse::print(res);
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

/**
*  ./client <REQ> --entry DIR --path . --name "" --content --edit-type hi
* argv[0] = .exe
* argv[1] = request type
* argv[2] = entry type
* argv[3] = path
* argv[4] = name (create, edit)
* argv[5] = content (create, edit)
* argv[6] = edit type (edit)
*/
std::vector<std::optional<std::string>> process_argv(int argc, char* argv[])
{
  std::vector<std::optional<std::string>> res;
  if(strcmp(argv[1], "GET") == 0)
    res.resize(3);

  else if(strcmp(argv[1], "CREATE") == 0)
    res.resize(5);

  else if(strcmp(argv[1], "EDIT") == 0)
    res.resize(6);

  else if(strcmp(argv[1], "REMOVE") == 0)
    res.resize(3);
  else
  {
    std::stringstream ss;
    ss << "Unrecognized request type: " << argv[1] << std::endl;
    err_exit(ss.str());
  }
  res[0] = argv[1];
  for(int i = 2; i < argc; i++)
  {
    if(strcmp(argv[i], "--entry") == 0)
      res[1] = argv[++i];

    else if(strcmp(argv[i], "--path") == 0)
    {
      if(argv[i + 1][0] == '"')
      {
        i++;
        std::stringstream buffer;
        buffer << std::string(argv[i++] + 1);
        i++;
        while(i + 1 < argc && argv[i][strlen(argv[i]) - 1] != '"')
          buffer << argv[i++];
        if (i < argc && argv[i][strlen(argv[i]) - 1] == '"')
        {
          buffer << " ";
          buffer << std::string(argv[i], strlen(argv[i]) - 1);
        }
        res[2] = buffer.str();
      }
      else
        res[2] = argv[++i];
    }
    else if(strcmp(argv[i], "--name") == 0)
    {
      if(res.size() >= 4)
      {
        if(argv[i + 1][0] == '"')
        {
          i++;
          std::stringstream buffer;
          buffer << std::string(argv[i++] + 1);
          i++;
          while(i + 1 < argc && argv[i][strlen(argv[i]) - 1] != '"')
            buffer << argv[i++];
          if (i < argc && argv[i][strlen(argv[i]) - 1] == '"')
          {
            buffer << " ";
            buffer << std::string(argv[i], strlen(argv[i]) - 1);
          }
          res[3] = buffer.str();
        }
        else
          res[3] = argv[++i];
      }
    }
    else if(strcmp(argv[i], "--content") == 0)
    {
      if(res.size() >= 5)
      {
        if(argv[i + 1][0] == '"')
        {
          i++;
          std::stringstream buffer;
          buffer << std::string(argv[i++] + 1);
          i++;
          while(i + 1 < argc && argv[i][strlen(argv[i]) - 1] != '"')
            buffer << argv[i++];
          if (i < argc && argv[i][strlen(argv[i]) - 1] == '"')
          {
            buffer << " ";
            buffer << std::string(argv[i], strlen(argv[i]) - 1);
          }
          res[4] = buffer.str();
        }
        else
          res[4] = argv[++i];
      }
    }
    else if(strcmp(argv[i], "--edit-type") == 0)
    {
      if(res.size() >= 6)
        res[5] = argv[++i];
    }
  }
  return res;
}

int main(int argc, char* argv[])
{
  if(argc == 2 && strcmp(argv[1], "?") == 0)
  {
    std::cout << "Usage: \n"
      << "GET [FILE/DIR] <path>\n"
      << "CREATE [FILE/DIR] <path> <name> <content (optional; only if file)>\n"
      << "EDIT [FILE/DIR] <path> <name (optional)> <content (optional)> [OVERRIDE/APPEND]\n"
      << "REMOVE [FILE/DIR] <path>" << std::endl;
    return EXIT_SUCCESS;
  }
  
  if(argc <= 2)
  {
    std::cerr << "Wrong usage,\nCorrect usage: ./client <request> <args...>\n"
      << "Use \"./client ?\" for more details" << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<std::optional<std::string>> args = process_argv(argc, argv);
  if(!args[1].has_value() || (args[1].value() != "FILE" && args[1].value() != "DIR"))
  {
    std::cerr << "Invalid entity: " << "\""
    << (args[1].has_value() ? args[1].value() : "")
    << "\"" << std::endl;
    return EXIT_FAILURE;
  }
  std::unordered_map<std::string, std::function<Req(std::vector<std::optional<std::string>>&)>> request_map = {
    {MessageType::GET, [&](std::vector<std::optional<std::string>>& vec) -> Request::Get {
      EntryType et = vec[1] == "FILE" ? EntryType::FILE : EntryType::DIR;
      std::string path = vec[2].value();
      auto req =  create_req<Request::Get>(et, path);
      return req;
    }},
    {MessageType::CREATE, [&](std::vector<std::optional<std::string>>& vec) -> Request::Create {
      EntryType et = vec[1] == "FILE" ? EntryType::FILE : EntryType::DIR;
      std::string path = vec[2].value(), name = vec[3].value();
      std::optional<std::string> content = vec[4].value();
      auto req =  create_req<Request::Create>(et, path, name, content);
      return req;
    }},
    {MessageType::EDIT, [&](std::vector<std::optional<std::string>>& vec) -> Request::Edit {
      EntryType et = vec[1] == "FILE" ? EntryType::FILE : EntryType::DIR;
      EditType ut = vec[5] == "OVERRIDE" ? EditType::OVERRIDE : EditType::APPEND;
      std::string path = vec[2].value();
      std::optional<std::string> name = vec[3], content = vec[4];
      auto req =  create_req<Request::Edit>(et, path, name, content, ut);
      return req;
    }},
    {MessageType::REMOVE, [&](std::vector<std::optional<std::string>>& vec) -> Request::Remove {
      EntryType et = vec[1] == "FILE" ? EntryType::FILE : EntryType::DIR;
      std::string path = vec[2].value();
      auto req =  create_req<Request::Remove>(et, path);
      return req;
    }}
  };
  Req request;
  if (request_map.find(args[0].value()) != request_map.end())
    request = request_map[args[0].value()](args);
  else
  {
    std::stringstream ss;
    ss << "Unknown request type: " << args[0].value();
    err_exit(ss.str());
  }
  
  SOCKET socket = create_socket();
  std::string str = Transformer::TRequest::serialize(request);
  handle_send(socket, str);
  handle_recv(socket);
  close_socket(socket);
  WSACleanup();
}