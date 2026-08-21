#include "rocklaunch/core/utils/checksum.h"

#include <cstdio>
#include <cstring>
#include <stdexcept>

#include <openssl/evp.h>

namespace rocklaunch
{

std::string HashFile(const fs::path &path, const EVP_MD *algo)
{
    if (algo == nullptr) {
        algo = EVP_sha512();
    }

    FILE *file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) {
        throw std::runtime_error("Cannot open file for hashing: " + path.string());
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        std::fclose(file);
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    if (EVP_DigestInit_ex(ctx, algo, nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        std::fclose(file);
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }

    unsigned char buffer[8192];
    size_t bytesRead = 0;
    while ((bytesRead = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (EVP_DigestUpdate(ctx, buffer, bytesRead) != 1) {
            EVP_MD_CTX_free(ctx);
            std::fclose(file);
            throw std::runtime_error("EVP_DigestUpdate failed");
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        std::fclose(file);
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }

    EVP_MD_CTX_free(ctx);
    std::fclose(file);

    char hex[EVP_MAX_MD_SIZE * 2 + 1];
    for (unsigned int i = 0; i < hashLen; ++i) {
        std::snprintf(hex + i * 2, 3, "%02x", hash[i]);
    }

    return std::string(hex, hashLen * 2);
}

} // namespace rocklaunch
