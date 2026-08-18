#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace rocklaunch
{

namespace fs = std::filesystem;

// Minimal PSARC (PlayStation Archive) reader/writer for Rocksmith 2014.
// Handles AES-256-CFB encrypted TOC and zlib-compressed data blocks.
namespace psarc_util
{

struct PsarcEntry
{
    uint8_t md5[16];
    uint32_t zIndex;    // index into the block size table
    uint64_t length;    // uncompressed size
    uint64_t offset;    // absolute byte offset in the psarc file
};

struct PsarcHeader
{
    char magic[4];          // "PSAR"
    uint32_t version;       // e.g. 0x00010004 (v1.4)
    char compression[4];    // "zlib"
    uint32_t tocSize;       // total TOC size including 32-byte header
    uint32_t entrySize;     // bytes per TOC entry (30)
    uint32_t numEntries;    // number of file entries
    uint32_t blockSize;     // decompression buffer size (65536)
    uint32_t archiveFlags;  // bit 2 = TOC encrypted
};

// Entry 0 (the manifest / file list) is included.
void Extract(const fs::path &psarcPath, const fs::path &destDir);

// Entry 0 is the manifest: a newline-separated list of relative paths for entries 1+.
void Repack(const fs::path &sourceDir, const fs::path &psarcPath);

} // namespace psarc_util
} // namespace rocklaunch
