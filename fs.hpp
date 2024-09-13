#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <fstream>
#include <filesystem>
#include "defs.hpp"

class FileSystem
{
  public:
  struct File
  {
    static std::optional<std::string> read(const std::string& path)
    {
      try
      {
        if (!std::filesystem::is_regular_file(path))
        {
          std::cerr << path << " is not a file" << std::endl;
          return std::nullopt;
        }
        std::ifstream file(path);
        if(!file.is_open())
        {
          std::cerr << "file " << path << " could not be opened.." << std::endl;
          return std::nullopt;
        }
        std::stringstream ss;
        std::string line;
        while(std::getline(file, line))
          ss << line;
        return ss.str();
      }
      catch (const std::filesystem::filesystem_error& e)
      {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return std::nullopt;
      }
    }
    static std::optional<std::string> create(const std::string& path, const std::string& name)
    {
      if (!std::filesystem::is_directory(path))
        return "Provided path is not a directory";
      if (!std::filesystem::exists(path))
      {
        std::cout << "Directory does not exist";
        return "Directory does not exist";
      }
      std::string full_path = path + "/" + name;
      std::fstream file(full_path, std::ios::out); 
      if(!file)
      { 
        std::cout << "Error in creating file"; 
        return "Error in creating file"; 
      } 
      file.close();
      return std::nullopt;
    }
    static std::optional<std::string> edit(
      const std::string& path,
      const std::string& name,
      const std::string& content,
      UpdateType ut
    )
    {
      if (!std::filesystem::is_regular_file(path))
        return "Provided path is not a file";

      std::filesystem::path new_path = std::filesystem::path(path).parent_path() / name;
      std::filesystem::rename(path, new_path);

      std::ofstream file(new_path, std::ios::out | std::ios::binary | (ut == UpdateType::APPEND ? std::ios::app : std::ios::trunc));
      if (!file)
      {
        std::cerr << "Failed to open the file for writing" << std::endl;
        return std::nullopt;
      }
      file << content;
      file.close();

      std::ifstream read_file(new_path, std::ios::in | std::ios::binary);
      if (!read_file)
      {
        std::cerr << "Failed to open the file for reading" << std::endl;
        return std::nullopt;
      }

      std::string res((std::istreambuf_iterator<char>(read_file)), std::istreambuf_iterator<char>());
      return res;
    }
    static std::optional<std::string> remove(const std::string& path)
    {
      if (!std::filesystem::is_regular_file(path))
        return "Provided path is not a file";
      if(!std::filesystem::remove(path))
        return "File deletion failed or file does not exist";
      return std::nullopt;
    }
  };
  struct Dir
  {
    static std::optional<std::string> read(const std::string& path)
    {
      try
      {
        if (!std::filesystem::is_directory(path))
        {
          std::cerr << path << " is not a directory" << std::endl;
          return std::nullopt;
        }
        std::stringstream ss;
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
          if (entry.is_directory())
            ss << "\n  📁 " << entry.path().filename().string();
          else if (entry.is_regular_file())
            ss << "\n  📄 " << entry.path().filename().string();
        }
        return ss.str();
      }
      catch (const std::filesystem::filesystem_error& e)
      {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return std::nullopt;
      }
    }
    static std::optional<std::string> create(const std::string& path, const std::string& name)
    {
      if (!std::filesystem::is_directory(path))
        return "Provided path is not a directory";
      std::filesystem::path full_path = std::filesystem::path(path) / name;
      try
      {
        if(!std::filesystem::create_directory(full_path))
        {
          std::cerr << "Directory already exists" << std::endl;
          return "Directory already exists";
        }
      }
      catch(std::filesystem::filesystem_error& e)
      {
        std::cerr << e.what() << std::endl;
        return e.what();
      }
      return std::nullopt;
    }
    static std::optional<std::string> edit(const std::string& path, const std::string& name)
    {
      try
      {
        if (!std::filesystem::is_directory(path))
          return "Provided path is not a directory";

        std::filesystem::path new_path = std::filesystem::path(path).parent_path() / name;
        std::filesystem::rename(path, new_path);

        return new_path.string();
      }
      catch (const std::filesystem::filesystem_error& e)
      {
        return e.what();
      }
      catch (const std::exception& e)
      {
        return e.what();
      }
    }
    static std::optional<std::string> remove(const std::string& path)
    {
      if (!std::filesystem::is_directory(path))
        return "Provided path is not a directory";
      if(!std::filesystem::remove_all(path))
        return "Directory deletion failed or directory does not exist";
      return std::nullopt;
    }
  };
};