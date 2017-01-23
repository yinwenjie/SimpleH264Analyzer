// CAVLCDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "CAVLC.h"
#include <iostream>

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	int coeffs[16] = { 3, 2, 1, -1, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
	string cavlc_code = Encoding_cavlc_16x16(coeffs);
	cout << cavlc_code << endl;
	return 0;
}

