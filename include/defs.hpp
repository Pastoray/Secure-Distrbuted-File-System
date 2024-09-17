#pragma once

#include <string>
#include <optional>
#include <variant>

#define BASE_PATH "C:/Users/pc/Desktop/folder/" // Example Base Path

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

enum class EditType
{
  OVERRIDE,
  APPEND
};

namespace Request
{
  struct Get
  {
    EntryType et;
    std::string path;
  };
  struct Create
  {
    EntryType et;
    std::string path;
    std::string name;
    std::optional<std::string> content; // if file
  };
  struct Edit
  {
    EntryType et;
    std::string path;
    std::optional<std::string> name;
    std::optional<std::string> content;
    EditType ut;
  };
  struct Remove
  {
    EntryType et;
    std::string path;
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
  struct Edit
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

using Res = std::variant<Response::Get, Response::Create, Response::Edit, Response::Remove>;
using Req = std::variant<Request::Get, Request::Create, Request::Edit, Request::Remove>;

void err_exit(const std::string& err)
{
  std::cerr << err << std::endl;
  exit(EXIT_FAILURE);
}
