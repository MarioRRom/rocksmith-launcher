#include "rocklaunch/core/psarc_util.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <openssl/evp.h>
#include <zlib.h>

namespace rocklaunch
{
namespace psarc_util
{

namespace
{

// Rocksmith PSARC AES-256-CFB key and IV (public, from rs-utils / Rocksmith Toolkit)
const uint8_t PSARC_KEY[32] = {
    0xC5, 0x3D, 0xB2, 0x38, 0x70, 0xA1, 0xA2, 0xF7,
    0x1C, 0xAE, 0x64, 0x06, 0x1F, 0xDD, 0x0E, 0x11,
    0x57, 0x30, 0x9D, 0xC8, 0x52, 0x04, 0xD4, 0xC5,
    0xBF, 0xDF, 0x25, 0x09, 0x0D, 0xF2, 0x57, 0x2C,
};

const uint8_t PSARC_IV[16] = {
    0xE9, 0x15, 0xAA, 0x01, 0x8F, 0xEF, 0x71, 0xFC,
    0x50, 0x81, 0x32, 0xE4, 0xBB, 0x4C, 0xEB, 0x42,
};

uint32_t ReadBE32(const uint8_t *buf)
{
    return (uint32_t(buf[0]) << 24) | (uint32_t(buf[1]) << 16)
         | (uint32_t(buf[2]) << 8)  |  uint32_t(buf[3]);
}

void WriteBE32(uint8_t *buf, uint32_t value)
{
    buf[0] = uint8_t(value >> 24);
    buf[1] = uint8_t(value >> 16);
    buf[2] = uint8_t(value >> 8);
    buf[3] = uint8_t(value);
}

uint16_t ReadBE16(const uint8_t *buf)
{
    return uint16_t((uint16_t(buf[0]) << 8) | uint16_t(buf[1]));
}

void WriteBE16(uint8_t *buf, uint16_t value)
{
    buf[0] = uint8_t(value >> 8);
    buf[1] = uint8_t(value);
}

uint64_t ReadBE40(const uint8_t *buf)
{
    uint64_t val = 0;
    for (int i = 0; i < 5; ++i) {
        val = (val << 8) | buf[i];
    }
    return val;
}

void WriteBE40(uint8_t *buf, uint64_t value)
{
    for (int i = 4; i >= 0; --i) {
        buf[i] = uint8_t(value & 0xFF);
        value >>= 8;
    }
}

std::array<uint8_t, 16> ComputeMD5(const uint8_t *data, size_t len)
{
    std::array<uint8_t, 16> result;
    unsigned int md5Len = 0;
    EVP_Digest(data, len, result.data(), &md5Len, EVP_md5(), nullptr);
    return result;
}

// Zero-pad data to a multiple of 16 bytes (AES block size).
// PSARC TOC sizes are always 16-byte aligned, so this adds zero bytes
// in practice — applied for spec compliance with community tools.
std::vector<uint8_t> PadData(const uint8_t *data, size_t len)
{
    size_t padded = len + (16 - (len % 16)) % 16;
    std::vector<uint8_t> result(data, data + len);
    result.resize(padded, 0);
    return result;
}

std::vector<uint8_t> AesDecrypt(const uint8_t *data, size_t len)
{
    auto padded = PadData(data, len);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create AES context");
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cfb128(), nullptr,
                           PSARC_KEY, PSARC_IV) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize AES decryption");
    }

    std::vector<uint8_t> output(padded.size() + EVP_CIPHER_block_size(EVP_aes_256_cfb128()));
    int outLen = 0;
    int totalLen = 0;

    if (EVP_DecryptUpdate(ctx, output.data(), &outLen,
                          padded.data(), static_cast<int>(padded.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES decryption update failed");
    }
    totalLen = outLen;

    if (EVP_DecryptFinal_ex(ctx, output.data() + totalLen, &outLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES decryption final failed");
    }
    totalLen += outLen;

    EVP_CIPHER_CTX_free(ctx);
    output.resize(totalLen);
    return output;
}

std::vector<uint8_t> AesEncrypt(const uint8_t *data, size_t len)
{
    auto padded = PadData(data, len);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create AES context");
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cfb128(), nullptr,
                           PSARC_KEY, PSARC_IV) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize AES encryption");
    }

    std::vector<uint8_t> output(padded.size() + EVP_CIPHER_block_size(EVP_aes_256_cfb128()));
    int outLen = 0;
    int totalLen = 0;

    if (EVP_EncryptUpdate(ctx, output.data(), &outLen,
                          padded.data(), static_cast<int>(padded.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES encryption update failed");
    }
    totalLen = outLen;

    if (EVP_EncryptFinal_ex(ctx, output.data() + totalLen, &outLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES encryption final failed");
    }
    totalLen += outLen;

    EVP_CIPHER_CTX_free(ctx);
    output.resize(totalLen);
    return output;
}

std::vector<uint8_t> ZlibDecompress(const uint8_t *data, size_t len)
{
    z_stream stream = {};
    if (inflateInit(&stream) != Z_OK) {
        throw std::runtime_error("zlib inflateInit failed");
    }

    stream.next_in = const_cast<Bytef *>(data);
    stream.avail_in = static_cast<uInt>(len);

    std::vector<uint8_t> output;
    uint8_t buf[65536];

    int ret;
    do {
        stream.next_out = buf;
        stream.avail_out = sizeof(buf);
        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&stream);
            throw std::runtime_error("zlib inflate failed: " + std::to_string(ret));
        }
        output.insert(output.end(), buf, buf + sizeof(buf) - stream.avail_out);
    } while (ret != Z_STREAM_END);

    inflateEnd(&stream);
    return output;
}

// Compresses data with zlib (best compression).
std::vector<uint8_t> ZlibCompress(const uint8_t *data, size_t len)
{
    z_stream stream = {};
    if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK) {
        throw std::runtime_error("zlib deflateInit failed");
    }

    stream.next_in = const_cast<Bytef *>(data);
    stream.avail_in = static_cast<uInt>(len);

    std::vector<uint8_t> output;
    uint8_t buf[65536];

    int ret;
    do {
        stream.next_out = buf;
        stream.avail_out = sizeof(buf);
        ret = deflate(&stream, Z_FINISH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            deflateEnd(&stream);
            throw std::runtime_error("zlib deflate failed: " + std::to_string(ret));
        }
        output.insert(output.end(), buf, buf + sizeof(buf) - stream.avail_out);
    } while (ret != Z_STREAM_END);

    deflateEnd(&stream);
    return output;
}

std::vector<uint8_t> ReadFile(const fs::path &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
        throw std::runtime_error("Failed to read file: " + path.string());
    }

    return buffer;
}

// Writes a buffer to a file atomically (write to .tmp, then rename).
// The temp file is created in the same directory as the target to ensure
// rename() works even across filesystems.
void WriteFileAtomic(const fs::path &path, const uint8_t *data, size_t len)
{
    fs::path tmpPath = path.parent_path() / (path.filename().string() + ".tmp");
    std::ofstream file(tmpPath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + tmpPath.string());
    }

    file.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(len));
    file.close();

    std::error_code ec;
    fs::rename(tmpPath, path, ec);
    if (ec) {
        throw std::runtime_error("Failed to rename " + tmpPath.string()
                                 + " to " + path.string() + ": " + ec.message());
    }
}

PsarcHeader ParseHeader(const uint8_t *data)
{
    PsarcHeader header;
    std::memcpy(header.magic, data, 4);
    header.version = ReadBE32(data + 4);
    std::memcpy(header.compression, data + 8, 4);
    header.tocSize = ReadBE32(data + 12);
    header.entrySize = ReadBE32(data + 16);
    header.numEntries = ReadBE32(data + 20);
    header.blockSize = ReadBE32(data + 24);
    header.archiveFlags = ReadBE32(data + 28);
    return header;
}

void ParseToc(const uint8_t *tocData, size_t tocLen,
              PsarcHeader &header,
              std::vector<PsarcEntry> &entries,
              std::vector<uint16_t> &blockSizes)
{
    entries.clear();
    blockSizes.clear();

    size_t pos = 0;
    for (uint32_t i = 0; i < header.numEntries; ++i) {
        if (pos + header.entrySize > tocLen) {
            throw std::runtime_error("TOC truncated at entry " + std::to_string(i));
        }

        PsarcEntry entry;
        std::memcpy(entry.md5, tocData + pos, 16);
        entry.zIndex = ReadBE32(tocData + pos + 16);
        entry.length = ReadBE40(tocData + pos + 20);
        entry.offset = ReadBE40(tocData + pos + 25);
        entries.push_back(entry);
        pos += header.entrySize;
    }

    // Remaining bytes are the block size table (uint16 each).
    while (pos + 2 <= tocLen) {
        blockSizes.push_back(ReadBE16(tocData + pos));
        pos += 2;
    }
}

std::vector<uint8_t> DecompressEntry(const uint8_t *psarcData, size_t psarcLen,
                                     const PsarcEntry &entry,
                                     const std::vector<uint16_t> &blockSizes,
                                     uint32_t blockSize)
{
    std::vector<uint8_t> result;
    result.reserve(static_cast<size_t>(entry.length));

    size_t pos = entry.offset;
    uint32_t zIdx = entry.zIndex;

    while (result.size() < entry.length && zIdx < blockSizes.size()) {
        uint16_t compressedSize = blockSizes[zIdx];

        if (compressedSize == 0) {
            // Uncompressed block (blockSizes[i] == 0 means full-size uncompressed block).
            size_t toRead = std::min(static_cast<size_t>(blockSize),
                                     psarcLen - pos);
            result.insert(result.end(), psarcData + pos, psarcData + pos + toRead);
            pos += blockSize;
        } else if (psarcData[pos] != 0x78) {
            // Not a zlib stream (CMF byte 0x78 missing) — stored raw.
            result.insert(result.end(), psarcData + pos, psarcData + pos + compressedSize);
            pos += compressedSize;
        } else {
            if (pos + compressedSize > psarcLen) {
                throw std::runtime_error("PSARC data truncated at block "
                                         + std::to_string(zIdx));
            }
            auto decompressed = ZlibDecompress(psarcData + pos, compressedSize);
            result.insert(result.end(), decompressed.begin(), decompressed.end());
            pos += compressedSize;
        }
        ++zIdx;
    }

    if (result.size() > entry.length) {
        result.resize(static_cast<size_t>(entry.length));
    }
    return result;
}

} // namespace

void Extract(const fs::path &psarcPath, const fs::path &destDir)
{
    auto psarcData = ReadFile(psarcPath);
    if (psarcData.size() < 32) {
        throw std::runtime_error("PSARC file too small: " + psarcPath.string());
    }

    PsarcHeader header = ParseHeader(psarcData.data());
    if (std::memcmp(header.magic, "PSAR", 4) != 0) {
        throw std::runtime_error("Invalid PSARC magic in: " + psarcPath.string());
    }

    // Read and decrypt the TOC.
    size_t tocDataSize = header.tocSize - 32;
    std::vector<uint8_t> decryptedToc;

    if (header.archiveFlags & 4) {
        decryptedToc = AesDecrypt(psarcData.data() + 32, tocDataSize);
    } else {
        decryptedToc.assign(psarcData.begin() + 32,
                            psarcData.begin() + 32 + tocDataSize);
    }

    std::vector<PsarcEntry> entries;
    std::vector<uint16_t> blockSizes;
    ParseToc(decryptedToc.data(), decryptedToc.size(),
             header, entries, blockSizes);

    fs::create_directories(destDir);

    // Entry 0 is the manifest: a newline-separated list of file paths.
    std::string manifest;
    if (!entries.empty()) {
        auto manifestData = DecompressEntry(psarcData.data(), psarcData.size(),
                                            entries[0], blockSizes, header.blockSize);
        manifest.assign(manifestData.begin(), manifestData.end());
    }

    std::vector<std::string> filePaths;
    {
        std::string line;
        for (char c : manifest) {
            if (c == '\n' || c == '\r') {
                if (!line.empty()) {
                    filePaths.push_back(line);
                    line.clear();
                }
            } else {
                line += c;
            }
        }
        if (!line.empty()) {
            filePaths.push_back(line);
        }
    }

    // Extract each entry (entries[1..N] correspond to filePaths[0..N-1]).
    for (size_t i = 1; i < entries.size(); ++i) {
        if (i - 1 >= filePaths.size()) {
            break;
        }

        auto content = DecompressEntry(psarcData.data(), psarcData.size(),
                                       entries[i], blockSizes, header.blockSize);

        fs::path outPath = destDir / filePaths[i - 1];
        fs::create_directories(outPath.parent_path());
        WriteFileAtomic(outPath, content.data(), content.size());
    }
}

void Repack(const fs::path &sourceDir, const fs::path &psarcPath)
{
    std::vector<fs::path> filePaths;
    for (auto &entry : fs::recursive_directory_iterator(sourceDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        fs::path rel = fs::relative(entry.path(), sourceDir);
        filePaths.push_back(rel);
    }
    std::sort(filePaths.begin(), filePaths.end());
    std::reverse(filePaths.begin(), filePaths.end());

    // Build manifest: newline-separated list of file paths (reversed sort,
    // matching the convention used by Rocksmith community tools).
    std::string manifest;
    for (auto &fp : filePaths) {
        manifest += fp.string() + "\n";
    }

    const uint32_t blockSize = 65536;
    const uint32_t entrySize = 30;

    struct RawEntry {
        std::vector<uint8_t> data;
        uint64_t uncompressedSize;
    };

    std::vector<RawEntry> rawEntries;
    rawEntries.reserve(filePaths.size() + 1);

    {
        rawEntries.push_back({
            std::vector<uint8_t>(manifest.begin(), manifest.end()),
            manifest.size()
        });
    }

    for (auto &fp : filePaths) {
        auto fileData = ReadFile(sourceDir / fp);
        auto fileSize = fileData.size();
        rawEntries.push_back({ std::move(fileData), fileSize });
    }

    uint32_t numEntries = static_cast<uint32_t>(rawEntries.size());

    // Compress each entry block-by-block: each block is an independent zlib stream.
    // When compression doesn't help (compressed >= raw), store uncompressed
    // (blockSizes entry = 0, reader reads full blockSize bytes as-is).
    struct CompressedBlock {
        std::vector<uint8_t> data;
        bool compressed; // true if zlib-compressed, false if stored raw
    };

    std::vector<std::vector<CompressedBlock>> entryBlocks; // entryBlocks[entry][block]
    std::vector<uint16_t> blockSizes;
    std::vector<uint32_t> entryZIndices;
    std::vector<uint64_t> entryOffsets;
    std::vector<uint64_t> entryCompressedSizes; // total stored size per entry

    uint32_t zIdx = 0;
    for (auto &re : rawEntries) {
        entryZIndices.push_back(zIdx);
        entryOffsets.push_back(0); // placeholder, set below

        std::vector<CompressedBlock> blocks;
        uint64_t totalStored = 0;

        if (re.data.empty()) {
            // Empty entry: one uncompressed block of size 0.
            blocks.push_back({ {}, false });
            blockSizes.push_back(0);
            totalStored = 0;
            zIdx += 1;
        } else {
            for (size_t pos = 0; pos < re.data.size(); pos += blockSize) {
                size_t chunkLen = std::min(static_cast<size_t>(blockSize),
                                          re.data.size() - pos);
                auto compressed = ZlibCompress(re.data.data() + pos, chunkLen);

                // Store uncompressed if compression doesn't help or would
                // overflow uint16_t block size (max 65535, but blockSize is 65536).
                if (compressed.size() < chunkLen
                    && compressed.size() <= std::numeric_limits<uint16_t>::max()) {
                    blockSizes.push_back(static_cast<uint16_t>(compressed.size()));
                    totalStored += compressed.size();
                    blocks.push_back({ std::move(compressed), true });
                } else {
                    // Store raw: blockSizes = 0 for full blocks (readers use
                    // blockSize), actual size for partial blocks.
                    uint16_t rawSize = static_cast<uint16_t>(chunkLen % blockSize);
                    blockSizes.push_back(rawSize);
                    totalStored += chunkLen;
                    std::vector<uint8_t> rawChunk(re.data.data() + pos,
                                                  re.data.data() + pos + chunkLen);
                    blocks.push_back({ std::move(rawChunk), false });
                }
                zIdx += 1;
            }
        }

        entryBlocks.push_back(std::move(blocks));
        entryCompressedSizes.push_back(totalStored);
    }

    uint32_t tocSize = 32 + numEntries * entrySize
                     + static_cast<uint32_t>(blockSizes.size()) * 2;

    uint64_t dataOffset = tocSize;
    for (size_t i = 0; i < rawEntries.size(); ++i) {
        entryOffsets[i] = dataOffset;
        dataOffset += entryCompressedSizes[i];
    }

    std::vector<uint8_t> tocEntries;
    tocEntries.reserve(numEntries * entrySize);
    for (uint32_t i = 0; i < numEntries; ++i) {
        uint8_t entry[30] = {};

        // MD5: zero for entry 0 (manifest), MD5(filename) for entries 1+.
        if (i == 0) {
            std::memset(entry, 0, 16);
        } else {
            size_t nameIdx = i - 1;
            if (nameIdx < filePaths.size()) {
                // PSARC spec: MD5 is computed on the lowercased path.
                std::string nameStr = filePaths[nameIdx].generic_string();
                std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                auto md5 = ComputeMD5(reinterpret_cast<const uint8_t *>(nameStr.data()),
                                      nameStr.size());
                std::memcpy(entry, md5.data(), 16);
            }
        }

        WriteBE32(entry + 16, entryZIndices[i]);
        WriteBE40(entry + 20, rawEntries[i].uncompressedSize);
        WriteBE40(entry + 25, entryOffsets[i]);
        tocEntries.insert(tocEntries.end(), entry, entry + entrySize);
    }

    std::vector<uint8_t> blockTable;
    blockTable.reserve(blockSizes.size() * 2);
    for (uint16_t bs : blockSizes) {
        uint8_t buf[2];
        WriteBE16(buf, bs);
        blockTable.insert(blockTable.end(), buf, buf + 2);
    }

    uint8_t headerBuf[32] = {};
    std::memcpy(headerBuf, "PSAR", 4);
    WriteBE32(headerBuf + 4, 0x00010004);       // version 1.4
    std::memcpy(headerBuf + 8, "zlib", 4);      // compression
    WriteBE32(headerBuf + 12, tocSize);          // TOC size (includes header)
    WriteBE32(headerBuf + 16, entrySize);        // entry size
    WriteBE32(headerBuf + 20, numEntries);       // number of entries
    WriteBE32(headerBuf + 24, blockSize);        // block size
    WriteBE32(headerBuf + 28, 4);                // archiveFlags = encrypted

    // Assemble full TOC: header + entries + block table, then encrypt the data portion.
    std::vector<uint8_t> tocPlain;
    tocPlain.insert(tocPlain.end(), headerBuf, headerBuf + 32);
    tocPlain.insert(tocPlain.end(), tocEntries.begin(), tocEntries.end());
    tocPlain.insert(tocPlain.end(), blockTable.begin(), blockTable.end());

    std::vector<uint8_t> encryptedToc = AesEncrypt(
        tocPlain.data() + 32, tocPlain.size() - 32);

    std::ofstream outFile(psarcPath, std::ios::binary);
    if (!outFile.is_open()) {
        throw std::runtime_error("Cannot create PSARC: " + psarcPath.string());
    }

    outFile.write(reinterpret_cast<const char *>(headerBuf), 32);
    outFile.write(reinterpret_cast<const char *>(encryptedToc.data()),
                  static_cast<std::streamsize>(encryptedToc.size()));

    for (auto &blocks : entryBlocks) {
        for (auto &block : blocks) {
            outFile.write(reinterpret_cast<const char *>(block.data.data()),
                          static_cast<std::streamsize>(block.data.size()));
        }
    }

    outFile.close();
}

} // namespace psarc_util
} // namespace rocklaunch
