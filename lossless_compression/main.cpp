//#include "huffman_encoding.h"
#include "vogel_encoding.h"
#include <fstream>
#include <iostream>

int main() {
	std::ifstream myfile;
	myfile.open("book.txt");

	BYTE buffer[65536];
	uint32_t buffer_len;

	myfile.read((char*)buffer, 65536);
	buffer_len = myfile.gcount();

	BYTE c2[65536];
	UINT cl2;

	BYTE c3[65536];
	UINT cl3;

	//std::cout << "raw: " << std::endl;
	//for (size_t i = 0; i < buffer_len; i++) { std::cout << buffer[i]; }
	//std::cout << std::endl << std::endl;

	vogel_compress_chunk(buffer, buffer_len, c2, &cl2,10);

	/*
	std::cout << "compressed: " << std::endl;
	//for (size_t i = 0; i < cl2; i++){std::cout << c2[i] << " ";}
	std::cout << std::endl << std::endl;

	decompress_chunk(c2, cl2, c3, &cl3);

	std::cout << "decompressed: " << std::endl;
	for (size_t i = 0; i < cl3; i++) { std::cout << c3[i]; }
	std::cout << std::endl << std::endl;

	std::cout << buffer_len << " to " << cl2 << " bytes" << std::endl;
	*/

	return 0;
}