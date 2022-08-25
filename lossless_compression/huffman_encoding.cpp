#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <list>
#include <bitset>

typedef unsigned char BYTE;
typedef unsigned short DBYTE;
typedef unsigned int UINT;
typedef std::string string;

struct sort_map
{
	UINT freq;
	BYTE val;
};

struct huff_node
{
	UINT weight;
	BYTE val;
	huff_node* left;
	huff_node* right;
};

struct huff_frame
{
	huff_node* node;
	DBYTE code;
	BYTE code_len;
};

//to write the buffer must be 0s
class bit_stream
{
public:
	BYTE* buffer;
	BYTE* currbyte;
	BYTE spinner;

	bit_stream(BYTE* b)
	{
		buffer = b;
		currbyte = b;
		spinner = 0;
	}

	void write_bit(BYTE bit)
	{
		bit = !!bit; //booleanize
		*currbyte = (*currbyte & ~(1 << (7 - spinner))) | ( bit << (7 - spinner) ) ; //changing nth bit
		spinner++;
		if (spinner == 8)
		{
			spinner = 0;
			currbyte++;
		}
	}

	BYTE read_bit()
	{
		BYTE bit = *currbyte & (1 << (7 - spinner)); //reading nth bit
		bit = !!bit; //booleanize
		spinner++;
		if (spinner == 8)
		{
			spinner = 0;
			currbyte++;
		}
		return bit;
	}

	UINT bits_touched() { return (currbyte - buffer) * 8 + spinner; }
	UINT bytes_touched() { return (bits_touched() + 7) / 8; }
};

/*
** debug stuff
*/
std::vector<std::vector<BYTE>> mad;
void print_subtree(huff_node* r, int depth) {
	if (depth >= mad.size()) { std::vector<BYTE> tmp; mad.push_back(tmp); }
	mad[depth].push_back(r->val);
	if (r->left != NULL) { print_subtree(r->left, depth + 1);  print_subtree(r->right, depth + 1); }
}

void print_tree(huff_node* r) {
	mad.clear();
	std::vector<BYTE> start;
	mad.push_back(start);

	print_subtree(r, 0);

	for (BYTE i = 0; i < mad.size(); i++)
	{
		for (BYTE j = 0; j < mad[i].size(); j++)
		{
			std::cout << "|" << (BYTE)mad[i][j] << "|  ";
		}
		std::cout << std::endl;
	}
}

UINT compress(BYTE* src, uint32_t src_size, BYTE* dst, uint32_t* dst_size)
{
	sort_map freq_map[256];
	DBYTE code[256];
	BYTE code_len[256] = {0};

	//set memory
	for (size_t i = 0; i < 256; i++)
	{
		freq_map[i].val = i;
		freq_map[i].freq = 0;
		code_len[i] = 0;
	}

	//record char frequencies
	for (size_t i = 0; i < src_size; i++)
	{
		freq_map[src[i]].freq++;
	}

	//sort by map by frequency
	std::sort(freq_map, freq_map + 256, [](sort_map a, sort_map b) {return a.freq < b.freq; });

	//find beginning of values to map
	UINT last_freq = 0;
	for (size_t i = 0; i < 256; i++)
	{
		if (freq_map[i].freq != 0)
		{
			last_freq = i;
			break;
		}
	}

	/*
	**	generate huffman tree
	*/
	std::list<huff_node*> q1;
	std::list<huff_node*> q2;

	for (size_t i = last_freq; i < 256; i++)
	{
		huff_node* nodeptr = new huff_node{ freq_map[i].freq, freq_map[i].val, NULL, NULL};
		q1.push_back(nodeptr);
	}

	UINT s1,s2,s3;
	UINT w1, w2, w3, w4;
	while (q1.size() + q2.size() > 1)
	{
		if (q1.size() > 1) { w1 = (*q1.begin())->weight; w2 = (*++q1.begin())->weight;}
		else if (q1.size() > 0) { w1 = (*q1.begin())->weight; w2 = UINT32_MAX/2; }
		else { w1 = UINT32_MAX/2, w2 = UINT32_MAX/2; }

		if (q2.size() > 1) { w3 = (*q2.begin())->weight; w4 = (*++q2.begin())->weight; }
		else if (q2.size() > 0) { w3 = (*q2.begin())->weight; w4 = UINT32_MAX/2; }
		else { w3 = UINT32_MAX/2, w4 = UINT32_MAX/2; }

		s1 = w1 + w2;
		s2 = w3 + w4;
		s3 = w1 + w3;

		huff_node* tmp;
		if (s1 < s2 && s1 < s3) { tmp = new huff_node{ s1, NULL, *(q1.begin()), *(++q1.begin()) }; q1.pop_front(); q1.pop_front(); } //might have to switch order of children
		else if (s2 < s3) { tmp = new huff_node{ s2, NULL, *(q2.begin()), *(++q2.begin()) }; q2.pop_front(); q2.pop_front(); }
		else { tmp =  new huff_node{ s3, NULL, *(q1.begin()), *(q2.begin()) }; q1.pop_front(); q2.pop_front();}
		q2.push_back(tmp);
	}

	/*
	** pre-order traverse huffman tree
	*/
	std::list<huff_frame> q3;
	std::list<huff_frame> q4;

	huff_frame root = { q2.front() ,0 ,0};
	q3.push_back(root);

	while (!q3.empty())
	{
		huff_frame tmp = q3.front();
		q3.pop_front();
		if (tmp.node->left != NULL) 
		{
			huff_frame leftchild = { tmp.node->left, tmp.code , tmp.code_len + 1 };
			huff_frame rightchild = { tmp.node->right, tmp.code | (1 << tmp.code_len), tmp.code_len + 1 };
			q3.push_front(rightchild);
			q3.push_front(leftchild);
		}
		else
		{
			UINT val = tmp.node->val;
			code[val] = tmp.code;
			code_len[val] = tmp.code_len;
		}
		q4.push_back(tmp);
	}

	/*
	** encode tree and data
	**
	** 2 bytes for number of tree nodes
	** 4 bytes for length of data in bits
	** (q4.size() - 1) / 8 bytes for tree leaf/node flags
	** then leaf values
	** then data 
	*/
	bit_stream flag_stream(dst + 6);
	DBYTE nodes = q4.size();
	BYTE* leaf_vals = dst + 6 + (nodes - 1) / 8 + 1;

	for (auto iter = q4.begin(); iter != q4.end(); ++iter)
	{
		huff_frame tmp = *iter;
		BYTE leaf = tmp.node->left == NULL;
		flag_stream.write_bit(leaf);
		if (leaf) { *leaf_vals++ = tmp.node->val; }
		delete tmp.node;
	}

	bit_stream data_stream(leaf_vals); 

	for (size_t i = 0; i < src_size; i++)
	{
		BYTE l = code_len[src[i]];
		DBYTE c = code[src[i]];
		for (int i = 0; i < l; i++)
		{
			data_stream.write_bit((1 << i) & c);
		}
	}

	*dst_size = (leaf_vals - dst) + data_stream.bytes_touched();
	*(DBYTE*)dst = (DBYTE)nodes;
	*(UINT*)(dst + 2) = data_stream.bits_touched();

	return 0;
}

UINT decompress(BYTE* src, uint32_t src_size, BYTE* dst, uint32_t* dst_size)
{
	DBYTE nodes = *(DBYTE*)src;
	UINT data_bits = *(UINT*)(src+2);

	/*
	** reconstruct tree
	*/
	std::list<huff_node*> q1;
	bit_stream flag_stream(src + 6);
	BYTE* leaf_vals = src + 6 + (nodes - 1) / 8 + 1;

	huff_node* root = new huff_node{ flag_stream.read_bit(), 0, NULL, NULL };
	q1.push_front(root);

	while (!q1.empty())
	{
		huff_node* parent = q1.front();

		BYTE leaf = flag_stream.read_bit();
		BYTE val;
		huff_node* tmp;
		(leaf) ? (val = *leaf_vals++) : (val = 0);
		tmp = new huff_node{ 0, val, NULL, NULL };

		if (parent->left == NULL) { parent->left = tmp; }
		else if (parent->right == NULL) {
			parent->right = tmp;
			q1.pop_front();
		}

		if (!leaf) { q1.push_front(tmp); }
	}

	/*
	** decode data 
	*/
	bit_stream data_stream(leaf_vals);
	UINT written = 0;
	huff_node* currnode = root;
	BYTE* currbyte = dst;

	for (UINT i = 0; i < data_bits; i++)
	{
		BYTE bit = data_stream.read_bit();
		if (bit) { currnode = currnode->right; }
		else { currnode = currnode->left; }
		if (currnode->left == NULL) {
			*currbyte++ = currnode->val;
			currnode = root;
		}
	}

	*dst_size = currbyte - dst;

	return 0;
}

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
	
	std::cout << "raw: " << std::endl;
	for (size_t i = 0; i < buffer_len; i++){ std::cout << buffer[i]; }
	std::cout << std::endl << std::endl;
	
	compress(buffer, buffer_len, c2, &cl2);

	std::cout << "compressed: " << std::endl;
	//for (size_t i = 0; i < cl2; i++){std::cout << c2[i] << " ";}
	std::cout << std::endl << std::endl;

	decompress(c2, cl2, c3, &cl3);
	
	std::cout << "decompressed: " << std::endl;
	for (size_t i = 0; i < cl3; i++) { std::cout << c3[i]; }
	std::cout << std::endl << std::endl;
	
	std::cout << buffer_len << " to " << cl2 << " bytes" <<std::endl;

	return 0;
}