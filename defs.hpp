#pragma once

#include <string>
#include <optional>
#include <variant>

#define BASE_PATH "C:/Users/pc/Desktop/folder/"

struct MessageType
{
  static const std::string GET;
  static const std::string CREATE;
  static const std::string EDIT;
  static const std::string REMOVE;
};

const std::string MessageType::GET = "GET";
const std::string MessageType::CREATE = "CREATE";
const std::string MessageType::EDIT = "EDIT";
const std::string MessageType::REMOVE = "REMOVE";

enum class EntryType
{
  FILE,
  DIR
};

enum class UpdateType
{
  OVERRIDE,
  APPEND
};

namespace Request
{
  struct Get
  {
    std::string path;
    EntryType et;
  };
  struct Create
  {
    std::string path;
    EntryType et;
    std::string name;
    std::optional<std::string> content; // if file
  };
  struct Update
  {
    std::string path;
    EntryType et;
    UpdateType ut;
    std::optional<std::string> name;
    std::optional<std::string> content;
  };
  struct Remove
  {
    std::string path;
    EntryType et;
  };
}

namespace Response
{
  struct Get
  {
    bool ok;
    std::string msg;
    std::optional<std::string> content; // if ok
    EntryType et;
  };
  struct Create
  {
    bool ok;
    std::string msg;
  };
  struct Update
  {
    bool ok;
    std::string msg;
    std::optional<std::string> content; // if ok
  };
  struct Remove
  {
    bool ok;
    std::string msg;
  };
}

using Res = std::variant<Response::Get, Response::Create, Response::Update, Response::Remove>;
using Req = std::variant<Request::Get, Request::Create, Request::Update, Request::Remove>;
