#include "stdafx.h"
#include "Utils.h"

//********************Parsing Bitstream**********************
// Get bool value from bit position..
int Get_bit_at_position(UINT8 *buf, UINT32 &bytePosition, UINT8 &bitPosition)
{
	UINT8 mask = 0, val = 0;

	mask = 1 << (7 - bitPosition);
	val = ((buf[bytePosition] & mask) != 0);
	if (++bitPosition > 7)
	{
		bytePosition++;
		bitPosition = 0;
	}

	return val;
}

// Parse bit stream using Expo-Columb coding
int Get_uev_code_num(UINT8 *buf, UINT32 &bytePosition, UINT8 &bitPosition)
{
	assert(bitPosition < 8);
	UINT8 val = 0, prefixZeroCount = 0;
	int prefix = 0, surfix = 0;

	while (true)
	{
		val = Get_bit_at_position(buf, bytePosition, bitPosition);
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
		val = Get_bit_at_position(buf, bytePosition, bitPosition);
		surfix += val * (1 << (prefixZeroCount - i - 1));
	}

	prefix += surfix;

	return prefix;
}

// Parse bit stream using signed-Expo-Columb coding
int Get_sev_code_num(UINT8 *buf, UINT32 &bytePosition, UINT8 &bitPosition)
{
	int uev = Get_uev_code_num(buf, bytePosition, bitPosition);
	int sign = (uev % 2) ? 1 : -1;
	int sev = sign * ((uev + 1) >> 1);
	
	return sev;
}

// Parse bit stream as unsigned int bits
int Get_uint_code_num(UINT8 *buf, UINT32 &bytePosition, UINT8 &bitPosition, UINT8 length)
{
	UINT32 uVal = 0;

	for (int idx = 0; idx < length; idx++)
	{
		uVal += Get_bit_at_position(buf, bytePosition, bitPosition) << (length - idx - 1);
	}

	return uVal;
}
int Peek_uint_code_num(UINT8 *buf, UINT32 bytePosition, UINT8 bitPosition, UINT8 length)
{
	UINT32 uVal = 0;

	for (int idx = 0; idx < length; idx++)
	{
		uVal += Get_bit_at_position(buf, bytePosition, bitPosition) << (length - idx - 1);
	}

	return uVal;
}


// Parse bit stream as me(coded_block_pattern)
int Get_me_code_num(UINT8 *buf, UINT32 &bytePosition, UINT8 &bitPosition, UINT8 mode)
{
	int intra_cbp[48] = { 47, 31, 15, 0, 23, 27, 29, 30, 7, 11, 13, 14, 39, 43, 45, 46, 16, 3, 5, 10, 12, 19, 21, 26, 28, 35, 37, 42, 44, 1, 2 ,4, 8, 17,18,20,24,6,9,22,25,32,33,34,36,40,38,41};
	int uev = Get_uev_code_num(buf, bytePosition, bitPosition);
	return intra_cbp[uev];
}
//***********************************************************


int Extract_single_nal_unit(const char* fileName, UINT8 *nalBuf, UINT32 nalLen)
{
	if (!fileName)
	{
		return -1;
	}

	std::ofstream nal;
	nal.open(fileName, std::ios_base::binary);
	if (!nal.is_open())
	{
		return -3;
	}

	const char nalStartCode[4] = { 0,0,0,1 };
	nal.write(nalStartCode, 4);
	nal.write(reinterpret_cast<const char* >(nalBuf), nalLen);

	nal.close();
	return 0;
}
