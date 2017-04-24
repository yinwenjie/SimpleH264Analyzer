/*
将长度为16的一维数组按照zig-zag顺序，分16次插入到16×16矩阵中；
*/
#include <stdio.h>

int coeff[16][16] = { 0 };

const int SNGL_SCAN[16][2] =
{
	{ 0, 0 }, { 1, 0 }, { 0, 1 }, { 0, 2 },
	{ 1, 1 }, { 2, 0 }, { 3, 0 }, { 2, 1 },
	{ 1, 2 }, { 0, 3 }, { 1, 3 }, { 2, 2 },
	{ 3, 1 }, { 3, 2 }, { 2, 3 }, { 3, 3 }
};

/*
函数名：insert_matrix
参数：
	(in/out)int (*matrix)[16]：矩阵起始地址
	(in)int *block：待插入数据
	(in)int start：在矩阵中插入的起始位置
	(in)int maxCoeffNum：系数的最大个数
	(in)int x：4x4矩阵水平索引
	(in)int y：4x4矩阵垂直索引
*/
void insert_matrix(int(*matrix)[16], int *block, int start, int maxCoeffNum, int x, int y)
{
	int row = 0, column = 0, startX = 4 * x, startY = 4 * y;
	for (int idx = 0, pos0 = start; idx < maxCoeffNum && pos0 < 16; idx++)
	{
		row = SNGL_SCAN[pos0][0] + startY;
		column = SNGL_SCAN[pos0][1] + startX;
		matrix[row][column] = block[idx];
		pos0++;
	}
}

int main()
{
	int block[16] = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5 };

	insert_matrix(coeff, block, 0, 16, 1, 0);

	return 0;
}