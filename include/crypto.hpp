#include <string>
#include <cstring>
#include <vector>
#include <iomanip>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <random>

class Crypto
{
  public:
    static std::string encrypt(const std::string &plaintext)
    {
      EVP_CIPHER_CTX *ctx;
      unsigned char iv[16];
      std::vector<unsigned char> ciphertext(plaintext.size() + 16);
      int len = 0, ciphertext_len = 0;

      if (!(ctx = EVP_CIPHER_CTX_new()))
        throw std::runtime_error("Failed to create context");

      if (!RAND_bytes(iv, sizeof(iv)))
        throw std::runtime_error("Failed to generate IV");

      if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char*)key.data(), iv) != 1)
        throw std::runtime_error("Failed to initialize encryption");

      if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, (unsigned char*)plaintext.data(), plaintext.size()) != 1)
        throw std::runtime_error("Failed to encrypt");
        
      ciphertext_len = len;

      if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1)
        throw std::runtime_error("Failed to finalize encryption");

      ciphertext_len += len;

      ciphertext.resize(ciphertext_len + sizeof(iv));
      std::memmove(ciphertext.data() + sizeof(iv), ciphertext.data(), ciphertext_len);
      std::memcpy(ciphertext.data(), iv, sizeof(iv));

      EVP_CIPHER_CTX_free(ctx);
      std::string result = ciphertext_to_string(ciphertext);
      return result;
    }
    static std::string decrypt(const std::string& ciphertextstr)
    {
      const std::vector<unsigned char> ciphertext = string_to_ciphertext(ciphertextstr);
      EVP_CIPHER_CTX *ctx;
      std::vector<unsigned char> decrypted(ciphertext.size());
      unsigned char iv[16];
      int len = 0, decrypted_len = 0;

      std::memcpy(iv, ciphertext.data(), sizeof(iv));

      if (!(ctx = EVP_CIPHER_CTX_new()))
        throw std::runtime_error("Failed to create context");

      if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char*)key.data(), iv) != 1)
        throw std::runtime_error("Failed to initialize decryption");

      if (EVP_DecryptUpdate(ctx, decrypted.data(), &len, ciphertext.data() + sizeof(iv), ciphertext.size() - sizeof(iv)) != 1)
        throw std::runtime_error("Failed to decrypt");

      decrypted_len = len;

      if (EVP_DecryptFinal_ex(ctx, decrypted.data() + len, &len) != 1)
        throw std::runtime_error("Failed to finalize decryption");

      decrypted_len += len;
      decrypted.resize(decrypted_len);
      EVP_CIPHER_CTX_free(ctx);

      return std::string(decrypted.begin(), decrypted.end());
    }
    static std::string gen_rand_key(size_t length)
    {
      const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
      std::random_device rd;
      std::mt19937 generator(rd());
      std::uniform_int_distribution<> distribution(0, characters.size() - 1);

      std::string key;
      for (size_t i = 0; i < length; i++)
        key += characters[distribution(generator)];
      return key;
  }
  private:
    static const std::string key;
    static std::string ciphertext_to_string(const std::vector<unsigned char> &ciphertext)
    {
      std::stringstream ss;
      for (unsigned char c : ciphertext)
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
      return ss.str();
    }

    static std::vector<unsigned char> string_to_ciphertext(const std::string &hex_str)
    {
      std::vector<unsigned char> ciphertext;
      for (size_t i = 0; i < hex_str.length(); i += 2)
      {
        std::string byte = hex_str.substr(i, 2);
        unsigned char c = static_cast<unsigned char>(strtol(byte.c_str(), nullptr, 16));
        ciphertext.push_back(c);
      }
      return ciphertext;
    }
    static std::string read_key(const std::string& path)
    {
      std::ifstream envFile(path);
      std::string line;
      std::string prefix = "AES_KEY=";

      if (envFile.is_open())
      {
        while (getline(envFile, line))
        {
            if (line.empty() || line[0] == '#')
              continue;

            if (line.find(prefix) == 0)
            {
              std::string key = line.substr(prefix.length());
              envFile.close();
              return key;
            }
        }
        envFile.close();
      }
      return "";
    }
};

const std::string Crypto::key = Crypto::read_key("./.env.local");
