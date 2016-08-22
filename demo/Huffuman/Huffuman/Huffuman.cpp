// Huffuman.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <string>

using namespace std;

//每一个符号定义为一个结构体，包括字符和出现频次
typedef struct
{
	unsigned char	character;
	unsigned int	frequency;
} CharNode;

static bool open_input_file(ifstream &input, const char *inputFileName)
{
	input.open(inputFileName);
	if (!input.is_open())
	{
		return false;
	}
	return true;
}

struct MinHeapNode
{
	char data;
	unsigned int freq;
	MinHeapNode *left, *right;
	MinHeapNode(char data, unsigned freq)
	{
		left = right = NULL;
		this->data = data;
		this->freq = freq;
	}
};
typedef struct MinHeapNode MinHeapNode;

struct compare
{
	bool operator()(MinHeapNode* l, MinHeapNode *r)
	{
		return (l->freq > r->freq);
	}
};

static void get_huffman_code(MinHeapNode *root, string code)
{
	if (!root)
	{
		return;
	}

	if (root->data != -1)
	{
		cout << root->data << " : " << code << endl;;
	}

	get_huffman_code(root->left, code + "0");
	get_huffman_code(root->right, code + "1");
}

int _tmain(int argc, _TCHAR* argv[])
{
	ifstream inputFile;
	if (!open_input_file(inputFile, "input.txt"))
	{
		cout << "Error: opening input file failed!" << endl;
		return -1;
	}

	char buf = inputFile.get();
	CharNode nodeArr[256] = { { 0, 0 } };
	while (inputFile.good())
	{
		cout << buf;
		nodeArr[buf].character = buf;
		nodeArr[buf].frequency++;
		buf = inputFile.get();
	}
	cout << endl;

	priority_queue<MinHeapNode*, vector<MinHeapNode*>, compare>  minHeap;
	for (int idx = 0; idx < 256; idx++)
	{
		if (nodeArr[idx].frequency > 0)
		{
			cout << "Node " << idx << ": [" << nodeArr[idx].character << ", " << nodeArr[idx].frequency << "]" << endl;
			minHeap.push(new MinHeapNode(nodeArr[idx].character, nodeArr[idx].frequency));
		}
	}

	MinHeapNode *leftNode = NULL, *rightNode = NULL, *topNode = NULL;
	while (minHeap.size() != 1)
	{
		leftNode = minHeap.top();
		minHeap.pop();

		rightNode = minHeap.top();
		minHeap.pop();

		topNode = new MinHeapNode(-1, leftNode->freq + rightNode->freq);
		topNode->left = leftNode;
		topNode->right = rightNode;
		minHeap.push(topNode);
	}

	get_huffman_code(topNode, "");

	inputFile.close();
	return 0;
}

