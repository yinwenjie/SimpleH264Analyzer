#ifndef _ARITHMETIC_CODER_H_
#define _ARITHMETIC_CODER_H_

#define FIRST_QUATER 2500
#define HALF_VALUE	 5000
#define THIRD_QUATER 7500
#define FULL_VALUE	 10000

typedef struct ARITHMETIC_ENCODER
{
	long low;
	long high;
	long bits_to_follow;
} ARITHMETIC_ENCODER;

int Encode_one_symbol(unsigned int symbol);

int Encode_stop();

#endif