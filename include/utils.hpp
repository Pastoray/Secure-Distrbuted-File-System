#pragma once

#include <iostream>
#include <optional>
#include <variant>
#include <string>
#include <sstream>
#include "defs.hpp"
#include "fs.hpp"

void close_socket(SOCKET socket)
{
  if (shutdown(socket, SD_BOTH) == SOCKET_ERROR)
    std::cerr << "Shutdown failed with error: " << WSAGetLastError() << std::endl;
  if (closesocket(socket) == SOCKET_ERROR)
    std::cerr << "Close socket failed with error: " << WSAGetLastError() << std::endl;
}

std::optional<std::string> streamline_path(std::string& path)
{
  std::vector<std::string> stack;
  std::stringstream ss(path);
  std::string token;
  while(std::getline(ss, token, '/'))
  {
    if(token == "..")
    {
      if(stack.empty())
        return std::nullopt;
      stack.pop_back();
    }
    else if(token == ".")
      continue;
    stack.push_back(token);
  }
  std::stringstream().swap(ss);
  for(int i = 0; i < stack.size(); i++)
  {
    ss << stack[i];
    if(i != stack.size() - 1)
      ss << "/";
  }
  return ss.str();
}

Res process_request(Req& req)
{
  struct Visitor
  {
    static Res operator()(Request::Get& req) 
    {
      Response::Get res = {
        .ok = true,
        .msg = "Retreived content successfully",
        .content = std::nullopt,
        .et = req.et
      };
      auto path = streamline_path(req.path);
      if(!path.has_value())
      {
        res.ok = false;
        res.msg = "Invalid path";
      }
      else
      {
        std::string full_path = BASE_PATH + path.value();
        std::optional<std::string> content = (
          req.et == EntryType::FILE ?
          FileSystem::File::read(full_path) :
          FileSystem::Dir::read(full_path)
        );
        if(!content.has_value())
        {
          res.ok = false;
          res.msg = "Couldn't read file with path: " + path.value();
          std::cerr << "Couldn't read file with path: " << path.value() << std::endl;
        }
        else
          res.content = content.value();
      }
      return res;
    }
    static Res operator()(Request::Create& req) 
    {
      std::stringstream ss;
      ss << (req.et == EntryType::FILE ? "File" : "Directory") << " created successfully";
      Response::Create res = {
        .ok = true,
        .msg = ss.str()
      };
      auto path = streamline_path(req.path);
      if(!path.has_value())
      {
        res.ok = false;
        res.msg = "Invalid path";
      }
      else
      {
        std::string full_path = BASE_PATH + path.value();
        auto err = (
          req.et == EntryType::FILE ?
          FileSystem::File::create(full_path, req.name) :
          FileSystem::Dir::create(full_path, req.name)
        );
        if(err.has_value())
        {
          res.ok = false;
          res.msg = err.value();
        }
      }
      return res;
    }
    static Res operator()(Request::Edit& req) 
    {
      std::stringstream ss;
      ss << "Updated " << (req.et == EntryType::FILE ? "file" : "directory") << " successfully";
      Response::Edit res = {
        .ok = true,
        .msg = ss.str(),
      };
      auto path = streamline_path(req.path);
      if(!path.has_value())
      {
        res.ok = false;
        res.msg = "Invalid path";
      }
      else
      {
        std::string full_path = BASE_PATH + path.value();
        std::optional<std::string> content = (
          req.et == EntryType::FILE ?
          FileSystem::File::edit(full_path, req.name, req.content, req.ut) :
          FileSystem::Dir::edit(full_path, req.name)
        );
        if(!content.has_value())
        {
          res.ok = false;
          res.msg = "Couldn't read file with path: " + path.value();
          std::cerr << "Couldn't read file with path: " << path.value() << std::endl;
        }
        else
          res.content = content.value();
      }
      return res;
    }
    static Res operator()(Request::Remove& req) 
    {
      std::stringstream ss;
      ss << "Removed " << (req.et == EntryType::FILE ? "file" : "directory") << " successfully";
      Response::Remove res = {
        .ok = true,
        .msg = ss.str(),
      };
      auto path = streamline_path(req.path);
      if(!path.has_value())
      {
        res.ok = false;
        res.msg = "Invalid path";
      }
      else
      {
        std::string full_path = BASE_PATH + path.value();
        std::optional<std::string> err = (
          req.et == EntryType::FILE ?
          FileSystem::File::remove(full_path) :
          FileSystem::Dir::remove(full_path)
        );
        if(err.has_value())
        {
          res.ok = false;
          res.msg = err.value();
          std::cerr << err.value() << std::endl;
        }
      }
      return res;
    }
  };
  return std::visit(Visitor{}, req);
}