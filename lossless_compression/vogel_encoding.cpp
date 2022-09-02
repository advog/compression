
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <list>
#include <bitset>
#include <functional>

typedef unsigned char BYTE;
typedef unsigned short DBYTE;
typedef unsigned int UINT;
typedef std::string string;

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
		*currbyte = (*currbyte & ~(1 << (7 - spinner))) | (bit << (7 - spinner)); //changing nth bit
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

struct tree_node {
	UINT parent_offset;
	UINT weight;
	BYTE val;
	BYTE depth;
	UINT sort_offset;
	UINT children_offset[16];

	tree_node(UINT a1, UINT a2, BYTE a3, BYTE a4, UINT a5) {
		parent_offset = a1;
		weight = a2;
		val = a3;
		depth = a4;
		sort_offset = a5;
		for (BYTE i = 0; i < 16; i++) { children_offset[i] = 0; }
	}
};

struct sort_node {
	UINT tree_offset;
	int score;
};

UINT vogel_compress_chunk(BYTE* src, uint32_t src_size, BYTE* dst, uint32_t* dst_size, UINT depth)
{
	if (src_size > UINT16_MAX) { return 1; }
	if (depth > 100) { return 1; }

	//bigass malloc, approx 600Mib
	tree_node* nodes = (tree_node*)malloc(sizeof(tree_node) * depth * 65536);
	UINT nodes_total = 0;
	
	std::function<void(UINT)> print_trace = [nodes](UINT offset)
	{
		UINT cur = offset;
		std::vector<BYTE> agg;
		std::vector<UINT> off;
		while (true)
		{
			tree_node node = nodes[cur];
			if (node.val == 0xff) { break; }
			else 
			{ 
				//std::cout << node.parent_offset << std::endl;
				agg.push_back(node.val);
				off.push_back(node.parent_offset);
				cur = node.parent_offset; 
			}
		}
		for (char i = agg.size() - 1; i >= 0; i--) { std::cout << std::bitset<4>(agg[i]) << " " << off[i] << " "; }
		std::cout << std::endl;
	};

	//split into nibbles
	UINT N = 2 * src_size;
	BYTE* nibbles = (BYTE*)malloc(N);
	for (UINT i = 0; i < src_size; i++)
	{
		nibbles[2 * i] = src[i] >> 4;
		nibbles[2 * i + 1] = src[i] & 0x0f;
	}

	/*
	** Construct the tree
	*/
	nodes[nodes_total++] = tree_node(0,0,0xff,0,0);
	for (UINT i = 0; i < N; i++) 
	{
		UINT cur_offset = 0;
		UINT new_offset = 0;

		BYTE cur_len = depth; //depth to search to, must be adjusted for when we approach end of nibbles
		if (depth + i > N) { cur_len = N - i; }
		
		for (BYTE j = 0; j < cur_len; j++)
		{
			BYTE nibble = nibbles[i + j];

			//if the cur_node has no child offset for the current nibble...
			if (nodes[cur_offset].children_offset[nibble] == 0) 
			{
				new_offset = nodes_total++; //get offset for new node
				nodes[new_offset] = tree_node(cur_offset, 0, nibble, (BYTE)(j+1), 0); //initiate new node
				nodes[cur_offset].children_offset[nibble] = new_offset; //add offset to new_node in prev_node
			}
			else 
			{
				new_offset = nodes[cur_offset].children_offset[nibble];
			}
			nodes[new_offset].weight++;
			cur_offset = new_offset;
		}
	}
	
	/*
	** repeated sorts to determine final set
	*/
	sort_node* sort_map = (sort_node*)malloc(sizeof(sort_node) * nodes_total);
	UINT sort_offset = 0;
	UINT* p_sort_offset = &sort_offset;

	//this is cursed
	std::function<void(UINT)> traverse_tree = [&, p_sort_offset, sort_map, nodes](UINT root_offset)
	{
		UINT sort_off = (*p_sort_offset)++; //get current sort_offset then iterate it
		tree_node& node = nodes[root_offset];
		
		sort_map[sort_off] = sort_node{root_offset, (int)(node.depth * node.weight)};
		node.sort_offset = sort_off;
		
		for (BYTE i = 0; i < 16; i++)
		{
			if (node.children_offset[i] != 0) { traverse_tree(node.children_offset[i]); }
		}
	};
	traverse_tree(0);

	//remove root from sort_len
	UINT sort_len = *p_sort_offset - 1;
	sort_map++;
	
	//sort
	std::sort(sort_map, sort_map+sort_len, [](sort_node& a,sort_node& b) { return a.score > b.score; });

	for (UINT i = 0; i < 100; i++)
	{
		std::cout << sort_map[i].score << " :: ";
		print_trace(sort_map[i].tree_offset);
	}
	
	//print_tree(0, nodes);

	return 0;
}

UINT vogel_decompress_chunk(BYTE* src, uint32_t src_size, BYTE* dst, uint32_t* dst_size)
{
	return 0;
}
