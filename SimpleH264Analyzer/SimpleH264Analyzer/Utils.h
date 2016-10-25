#ifndef _UTILS_H_
#define _UTILS_H_
#include "Global.h"

//********************Parsing Bitstream**********************
// Get bool value from bit position..
int Get_bit_at_position(UINT8 *buf, UINT8 &bytePosition, UINT8 &bitPosition);
// Parse bit stream using Expo-Columb coding
int Get_uev_code_num(UINT8 *buf, UINT8 &bytePosition, UINT8 &bitPosition);
// Parse bit stream using signed-Expo-Columb coding
int Get_sev_code_num(UINT8 *buf, UINT8 &bytePosition, UINT8 &bitPosition);
// Parse bit stream as unsigned int bits
int Get_uint_code_num(UINT8 *buf, UINT8 &bytePosition, UINT8 &bitPosition, UINT8 length);
//***********************************************************

int Extract_single_nal_unit(const char* fileName, UINT8 *nalBuf, UINT32 nalLen);

#endif