#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <stdlib.h>
#include <iomanip>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include "defs.hpp"

class Transformer
{
  public:
  struct TRequest
  {
    static std::string serialize(const Req& req) 
    {
      struct Visitor
      {
        static std::string operator()(const Request::Get& req) 
        {
          std::stringstream ss;
          ss << MessageType::GET.size() << "," << MessageType::GET
            << "|" << req.path.size() << "," << req.path
            << "|1," << (int)req.et;
          return ss.str();
        }
        static std::string operator()(const Request::Create& req) 
        {
          std::stringstream ss;
          ss << MessageType::CREATE.size() << "," << MessageType::CREATE
            << "|" << req.path.size() << "," << req.path
            << "|1," << (int)req.et
            << "|" << req.name.size() << "," << req.name
            << "|" << (req.content.has_value() ? req.content.value().size() : 0)
            << "," << (req.content.has_value() ? req.content.value() : "");
          return ss.str();
        }
        static std::string operator()(const Request::Edit& req) 
        {
          std::stringstream ss;
          ss << MessageType::EDIT.size() << "," << MessageType::EDIT
            << "|" << req.path.size() << "," << req.path
            << "|1," << (int)req.et
            << "|1," << (int)req.ut
            << "|" << (req.name.has_value() ? req.name.value().size() : 0)
            << "," << (req.name.has_value() ? req.name.value() : "")
            << "|" << (req.content.has_value() ? req.content.value().size() : 0)
            << "," << (req.content.has_value() ? req.content.value() : "");
          return ss.str();
        }
        static std::string operator()(const Request::Remove& req) 
        {
          std::stringstream ss;
          ss << MessageType::REMOVE.size() << "," << MessageType::REMOVE
            << "|" << req.path.size() << "," << req.path
            << "|1," << (int)req.et;
          return ss.str();
        }
      };
      return std::visit(Visitor{}, req);
    }
    static Req deserialize(const std::string& str)
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
        size_t size = std::stoi(buffer);
        buffer.clear();
        tokens.push_back(token.substr(i + 1, i + size));
      }
      Req req;
      if(tokens[0] == MessageType::GET)
      {
        req = Request::Get {
          .et = (EntryType)std::stoi(tokens[2]),
          .path = tokens[1]
        };
      }
      else if(tokens[0] == MessageType::CREATE)
      {
        req = Request::Create {
          .et = (EntryType)std::stoi(tokens[2]),
          .path = tokens[1],
          .name = tokens[3],
          .content = (tokens[4] != "" ? std::optional<std::string>(tokens[4]) : std::nullopt)
        };
      }
      else if(tokens[0] == MessageType::EDIT)
      {
        req = Request::Edit {
          .et = (EntryType)std::stoi(tokens[2]),
          .path = tokens[1],
          .name = tokens[4] != "" ? std::make_optional(tokens[4]) : std::nullopt,
          .content = tokens[5] != "" ? std::make_optional(tokens[5]) : std::nullopt,
          .ut = (EditType)std::stoi(tokens[3])
        };
      }
      else if(tokens[0] == MessageType::REMOVE)
      {
        req = Request::Remove {
          .et = (EntryType)std::stoi(tokens[2]),
          .path = tokens[1]
        };
      }
      return req;
    }
  };
  struct TResponse
  {
    static std::string serialize(const Res& res) 
    {
      struct Visitor
      {
        static std::string operator()(const Response::Get& res) 
        {
          std::stringstream ss;
          ss << MessageType::GET.size() << "," << MessageType::GET
            << "|1," << res.ok
            << "|" << res.msg.size() << "," << res.msg
            << "|1," << (int)res.et
            << "|" << (res.content.has_value() ? res.content.value().size() : 0)
            << "," << (res.content.has_value() ? res.content.value() : "");
          return ss.str();
        }
        static std::string operator()(const Response::Create& res) 
        {
          std::stringstream ss;
          ss << MessageType::CREATE.size() << "," << MessageType::CREATE
            << "|1," << res.ok
            << "|" << res.msg.size() << "," << res.msg;
          return ss.str();
        }
        static std::string operator()(const Response::Edit& res) 
        {
          std::stringstream ss;
          ss << MessageType::EDIT.size() << "," << MessageType::EDIT
            << "|1," << res.ok
            << "|" << res.msg.size() << "," << res.msg
            << "|" << (res.content.has_value() ? res.content.value().size() : 0)
            << "," << (res.content.has_value() ? res.content.value() : "");
          return ss.str();
        }
        static std::string operator()(const Response::Remove& res) 
        {
          std::stringstream ss;
          ss << MessageType::REMOVE.size() << "," << MessageType::REMOVE
            << "|1," << res.ok
            << "|" << res.msg.size() << "," << res.msg;
          return ss.str();
        }
      };
      return std::visit(Visitor{}, res);
    }
    static Res deserialize(const std::string& str) 
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
        size_t size = std::stoi(buffer);
        buffer.clear();
        tokens.push_back(token.substr(i + 1, i + size));
      }
      Res res;
      if(tokens[0] == MessageType::GET)
      {
        res = Response::Get {
          .ok = (bool)std::stoi(tokens[1]),
          .msg = tokens[2],
          .content = tokens[4],
          .et = (EntryType)std::stoi(tokens[3])
        };
      }
      else if(tokens[0] == MessageType::CREATE)
      {
        res = Response::Create {
          .ok = (bool)std::stoi(tokens[1]),
          .msg = tokens[2]
        };
      }
      else if(tokens[0] == MessageType::EDIT)
      {
        res = Response::Edit {
          .ok = (bool)std::stoi(tokens[1]),
          .msg = tokens[2],
          .content = tokens[3]
        };
      }
      else if(tokens[0] == MessageType::REMOVE)
      {
        res = Response::Remove {
          .ok = (bool)std::stoi(tokens[1]),
          .msg = tokens[2]
        };
      }
      return res;
    }
    static void print(const Res& res)
    {
      struct Visitor {
        void operator()(const Response::Get& res) {
          std::cout << "OK: " << res.ok << "\n"
                    << "Entry Type: " << (int)res.et << "\n"
                    << "Message: " << res.msg << "\n"
                    << "Content: " << (res.content.has_value() ? res.content.value() : "") << std::endl;
        }
        void operator()(const Response::Create& res) {
          std::cout << "OK: " << res.ok << "\n"
                    << "Message: " << res.msg << std::endl;
        }
        void operator()(const Response::Edit& res) {
          std::cout << "OK: " << res.ok << "\n"
                    << "Message: " << res.msg << "\n"
                    << "Content: " << (res.content.has_value() ? res.content.value() : "") << std::endl;
        }
        void operator()(const Response::Remove& res) {
          std::cout << "OK: " << res.ok << "\n"
                    << "Message: " << res.msg << std::endl;
        }
      };
      std::visit(Visitor{}, res);
    }
  };
};
