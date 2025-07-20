#ifndef HASHER_H
#define HASHER_H

#include <string>

class Hasher {
public:
    Hasher(const std::string& key);
    std::string encrypt(const std::string& plaintext);
    std::string decrypt(const std::string& ciphertext_base64);

private:
    std::string key;
    std::string iv; 
    std::string base64Encode(const unsigned char* data, size_t len);
    std::string base64Decode(const std::string& input);
};

#endif
