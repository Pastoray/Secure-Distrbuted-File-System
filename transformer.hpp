#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <sstream>
#include <vector>
#include "defs.hpp"

class Transformer
{
  public:
  struct Serialize
  {
    static std::string request(const Req& req) 
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
        static std::string operator()(const Request::Update& req) 
        {
          std::stringstream ss;
          ss << MessageType::EDIT.size() << "," << MessageType::EDIT
            << "|" << req.path.size() << "," << req.path
            << "|1," << (int)req.et
            << "|1," << (int)req.ut
            << "|" << req.content.size() << "," << req.content; // TODO: fix serial and deserial
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
    static std::string response(const Res& res) 
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
        static std::string operator()(const Response::Update& res) 
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
  };
  struct Deserialize
  {
    static Req request(const std::string& str)
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
          .path = tokens[1],
          .et = (EntryType)std::stoi(tokens[2])
        };
      }
      else if(tokens[0] == MessageType::CREATE)
      {
        req = Request::Create {
          .path = tokens[1],
          .et = (EntryType)std::stoi(tokens[2]),
          .name = tokens[3],
          .content = (tokens[4] != "" ? std::optional<std::string>(tokens[4]) : std::nullopt)
        };
      }
      else if(tokens[0] == MessageType::EDIT)
      {
        req = Request::Update {
          .path = tokens[1],
          .et = (EntryType)std::stoi(tokens[2]),
          .ut = (UpdateType)std::stoi(tokens[3]),
          .content = tokens[4]
        };
      }
      else if(tokens[0] == MessageType::REMOVE)
      {
        req = Request::Remove {
          .path = tokens[1],
          .et = (EntryType)std::stoi(tokens[2])
        };
      }
      return req;
    }
    static Res response(const std::string& str) 
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
        res = Response::Update {
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
  };
};