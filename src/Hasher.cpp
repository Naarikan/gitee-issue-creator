#include "Hasher.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <cstring>
#include <iostream>

Hasher::Hasher(const std::string& key) : key(key) {
    iv = "0123456789abcdef";
}

std::string Hasher::base64Encode(const unsigned char* data, size_t len) {
    BIO* bio, *b64;
    BUF_MEM* bufferPtr;
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); 
    BIO_write(bio, data, (int)len);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    return result;
}

std::string Hasher::base64Decode(const std::string& input) {
    BIO* bio, *b64;
    int decodeLen = (int)input.length();
    char* buffer = (char*)malloc(decodeLen);
    memset(buffer, 0, decodeLen);

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(input.c_str(), -1);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    int length = BIO_read(bio, buffer, decodeLen);
    std::string result(buffer, length);
    BIO_free_all(bio);
    free(buffer);
    return result;
}

std::string Hasher::encrypt(const std::string& plaintext) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    int len;
    int ciphertext_len;
    unsigned char ciphertext[1024];

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (const unsigned char*)key.c_str(), (const unsigned char*)iv.c_str()))
        throw std::runtime_error("EVP_EncryptInit_ex failed");

    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, (const unsigned char*)plaintext.c_str(), (int)plaintext.length()))
        throw std::runtime_error("EVP_EncryptUpdate failed");

    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        throw std::runtime_error("EVP_EncryptFinal_ex failed");

    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    return base64Encode(ciphertext, ciphertext_len);
}

std::string Hasher::decrypt(const std::string& ciphertext_base64) {
    std::string ciphertext = base64Decode(ciphertext_base64);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    int len;
    int plaintext_len;
    unsigned char plaintext[1024];

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (const unsigned char*)key.c_str(), (const unsigned char*)iv.c_str()))
        throw std::runtime_error("EVP_DecryptInit_ex failed");

    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, (const unsigned char*)ciphertext.c_str(), (int)ciphertext.length()))
        throw std::runtime_error("EVP_DecryptUpdate failed");

    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        throw std::runtime_error("EVP_DecryptFinal_ex failed");

    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    return std::string((char*)plaintext, plaintext_len);
}
