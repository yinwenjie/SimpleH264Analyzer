#ifndef _UTILS_H_
#define _UTILS_H_
#include "Global.h"

int Get_bit_at_position(UINT8 *buf, UINT8 &bytePotion, UINT8 &bitPosition);

int Get_uev_code_num(UINT8 *buf, UINT8 &bytePotion, UINT8 &bitPosition);

#endif