#pragma once

#include <filesystem>
#include <string>

#include <openssl/evp.h>

namespace rocklaunch
{

namespace fs = std::filesystem;

// Compute hex-encoded digest of a file using the given EVP_MD algorithm.
// Default is SHA-512. Throws on I/O or hash errors.
std::string HashFile(const fs::path &path,
                     const EVP_MD *algo = nullptr);

} // namespace rocklaunch
