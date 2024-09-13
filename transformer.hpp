#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <stdlib.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include "defs.hpp"

class Transformer
{
  public:
  struct Reques
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
        req = Request::Edit {
          .path = tokens[1],
          .et = (EntryType)std::stoi(tokens[2]),
          .name = tokens[3],
          .content = tokens[4],
          .ut = (UpdateType)std::stoi(tokens[5]),
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
    static std::string encrypt(const std::string& plaintext)
    {
      if (!key || std::strlen(key) != 64)
      {
        std::cout << key << std::endl;
        err_exit("Invalid or missing AES key (must be 32 bytes for AES-256)");
      }
      
      unsigned char iv[AES_BLOCK_SIZE];
      if (!RAND_bytes(iv, AES_BLOCK_SIZE))
        err_exit("Error generating random IV");

      AES_KEY encryptKey;
      if (AES_set_encrypt_key(reinterpret_cast<const unsigned char*>(key), 256, &encryptKey) < 0)
        err_exit("Error setting encryption key");

      int length = plaintext.size();
      int paddedLength = ((length / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;

      std::vector<unsigned char> inputBuffer(paddedLength);
      std::vector<unsigned char> outputBuffer(paddedLength);

      // Copy the plaintext into the input buffer and apply zero padding
      std::memcpy(inputBuffer.data(), plaintext.c_str(), length);
      std::memset(inputBuffer.data() + length, 0, paddedLength - length);

      // Encrypt the input buffer using CBC mode
      AES_cbc_encrypt(inputBuffer.data(), outputBuffer.data(), paddedLength, &encryptKey, iv, AES_ENCRYPT);

      // Prepend the IV to the encrypted data
      std::string encryptedData(reinterpret_cast<char*>(iv), AES_BLOCK_SIZE);  // IV is 16 bytes
      encryptedData.append(outputBuffer.begin(), outputBuffer.end());           // Append ciphertext

      return encryptedData;  // Return the combined IV + ciphertext
    }
    static std::string decrypt(const std::string& ciphertext)
    {
      if (!key || std::strlen(key) != 64)
      {
        std::cout << key << std::endl;
        err_exit("Invalid or missing AES key (must be 32 bytes for AES-256)");
      }

      if (ciphertext.size() < AES_BLOCK_SIZE)
        err_exit("Ciphertext too short");

      // Extract the IV from the first 16 bytes of the ciphertext
      unsigned char iv[AES_BLOCK_SIZE];
      std::memcpy(iv, ciphertext.data(), AES_BLOCK_SIZE);

      AES_KEY decryptKey;
      if (AES_set_decrypt_key(reinterpret_cast<const unsigned char*>(key), 256, &decryptKey) < 0)
        err_exit("Error setting decryption key");

      int ciphertextLength = ciphertext.size() - AES_BLOCK_SIZE;
      std::vector<unsigned char> inputBuffer(ciphertextLength);
      std::vector<unsigned char> outputBuffer(ciphertextLength);

      // Copy the ciphertext (after the IV) into the input buffer
      std::memcpy(inputBuffer.data(), ciphertext.data() + AES_BLOCK_SIZE, ciphertextLength);

      // Decrypt the input buffer using CBC mode
      AES_cbc_encrypt(inputBuffer.data(), outputBuffer.data(), ciphertextLength, &decryptKey, iv, AES_DECRYPT);

      // Convert the decrypted data to a string (trim padding if necessary)
      return std::string(outputBuffer.begin(), outputBuffer.end());
    }
  };
  struct Respons
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
    static std::string encrypt(const std::string& plaintext)
    {
      if (!key || std::strlen(key) != 64)
      {
        std::cout << key << std::endl;
        err_exit("Invalid or missing AES key (must be 32 bytes for AES-256)");
      }
      
      unsigned char iv[AES_BLOCK_SIZE];
      if (!RAND_bytes(iv, AES_BLOCK_SIZE))
        err_exit("Error generating random IV");

      AES_KEY encryptKey;
      if (AES_set_encrypt_key(reinterpret_cast<const unsigned char*>(key), 256, &encryptKey) < 0)
        err_exit("Error setting encryption key");

      int length = plaintext.size();
      int paddedLength = ((length / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;

      std::vector<unsigned char> inputBuffer(paddedLength);
      std::vector<unsigned char> outputBuffer(paddedLength);

      // Copy the plaintext into the input buffer and apply zero padding
      std::memcpy(inputBuffer.data(), plaintext.c_str(), length);
      std::memset(inputBuffer.data() + length, 0, paddedLength - length);

      // Encrypt the input buffer using CBC mode
      AES_cbc_encrypt(inputBuffer.data(), outputBuffer.data(), paddedLength, &encryptKey, iv, AES_ENCRYPT);

      // Prepend the IV to the encrypted data
      std::string encryptedData(reinterpret_cast<char*>(iv), AES_BLOCK_SIZE);  // IV is 16 bytes
      encryptedData.append(outputBuffer.begin(), outputBuffer.end());           // Append ciphertext

      return encryptedData;  // Return the combined IV + ciphertext
    }
    static std::string decrypt(const std::string& ciphertext)
    {
      if (!key || std::strlen(key) != 64)
      {
        std::cout << key << std::endl;
        err_exit("Invalid or missing AES key (must be 32 bytes for AES-256)");
      }

      if (ciphertext.size() < AES_BLOCK_SIZE)
        err_exit("Ciphertext too short");

      // Extract the IV from the first 16 bytes of the ciphertext
      unsigned char iv[AES_BLOCK_SIZE];
      std::memcpy(iv, ciphertext.data(), AES_BLOCK_SIZE);

      AES_KEY decryptKey;
      if (AES_set_decrypt_key(reinterpret_cast<const unsigned char*>(key), 256, &decryptKey) < 0)
        err_exit("Error setting decryption key");

      int ciphertextLength = ciphertext.size() - AES_BLOCK_SIZE;
      std::vector<unsigned char> inputBuffer(ciphertextLength);
      std::vector<unsigned char> outputBuffer(ciphertextLength);

      // Copy the ciphertext (after the IV) into the input buffer
      std::memcpy(inputBuffer.data(), ciphertext.data() + AES_BLOCK_SIZE, ciphertextLength);

      // Decrypt the input buffer using CBC mode
      AES_cbc_encrypt(inputBuffer.data(), outputBuffer.data(), ciphertextLength, &decryptKey, iv, AES_DECRYPT);

      // Convert the decrypted data to a string (trim padding if necessary)
      return std::string(outputBuffer.begin(), outputBuffer.end());
    }
  };
  private:
    static const char* key;
  public:
    static void set_key(const char* newKey)
    {
      // key = new char[strlen(newKey) + 1];
      // strcpy(key, newKey);
    };
};

const char* Transformer::key = "f33b77b35fe4d3e0ea2d11d673c891e9ad98a3cc1a0a1a45c20fa5ad9a34a7b7";

