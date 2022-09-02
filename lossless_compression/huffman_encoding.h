#pragma once
#include <string>

typedef unsigned char BYTE;
typedef unsigned short DBYTE;
typedef unsigned int UINT;
typedef std::string string;

UINT huffman_decompress_chunk(BYTE* src, uint32_t src_size, BYTE* dst, uint32_t* dst_size);
UINT huffman_compress_chunk(BYTE* src, uint32_t src_size, BYTE* dst, uint32_t* dst_size);
