// ExpColum.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <assert.h>

typedef unsigned char UINT8;

static int get_bit_at_position(UINT8 *buf, UINT8 &bytePotion, UINT8 &bitPosition)
{
	UINT8 mask = 0, val = 0;

	mask = 1 << (7 - bitPosition);
	val = ((buf[bytePotion] & mask) != 0);
	if (++bitPosition > 7)
	{
		bytePotion++;
		bitPosition = 0;
	}

	return val;
}

static int get_uev_code_num(UINT8 *buf, UINT8 &bytePotion, UINT8 &bitPosition)
{
	assert(bitPosition < 8);
	UINT8 val = 0, prefixZeroCount = 0;
	int prefix = 0, surfix = 0;

	while (true)
	{
		val = get_bit_at_position(buf, bytePotion, bitPosition);
		if (val == 0)
		{
			prefixZeroCount++;
		}
		else
		{
			break;
		}
	}
	prefix = (1 << prefixZeroCount) - 1;
	for (size_t i = 0; i < prefixZeroCount; i++)
	{
		val = get_bit_at_position(buf, bytePotion, bitPosition);
		surfix += val * (1 << (prefixZeroCount - i - 1));
	}

	prefix += surfix;

	return prefix;
}

int _tmain(int argc, _TCHAR* argv[])
{
	UINT8 strArray[6] = { 0xA6, 0x42, 0x98, 0xE2, 0x04, 0x8A };
	UINT8 bytePosition = 0, bitPosition = 0;
	UINT8 dataLengthInBits = sizeof(strArray) * 8;

	int codeNum = 0;
	while ((bytePosition * 8 + bitPosition) < dataLengthInBits)
	{
		codeNum = get_uev_code_num(strArray, bytePosition, bitPosition);
		printf("ExpoColumb codeNum = %d\n", codeNum);
	}

	return 0;
}

