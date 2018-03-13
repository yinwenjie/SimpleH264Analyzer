#include "ArithmeticCoder.h"

static unsigned int probability_model[3] = { 0, 7000, 10000 };
static unsigned int upper_bound = 10000;

ARITHMETIC_ENCODER encoder = { 0, FULL_VALUE, 0 };

void output_bits(int bit)
{
	printf("%d", bit);
	while (encoder.bits_to_follow > 0)
	{
		printf("%d", !bit);
		encoder.bits_to_follow--;
	}
}

int Encode_one_symbol(unsigned int symbol)
{
	long range = encoder.high - encoder.low + 1;
	encoder.high = encoder.low + (range * probability_model[symbol + 1] / upper_bound) - 1;
	encoder.low = encoder.low + (range * probability_model[symbol] / upper_bound);

	while (1)
	{
		
		if (encoder.high < HALF_VALUE)
		{
			// Output 0;
			output_bits(0);
		}
		else if (encoder.low >= HALF_VALUE)
		{
			// Output 1;
			output_bits(1);
			encoder.high -= HALF_VALUE;
			encoder.low -= HALF_VALUE;
		}
		else if (encoder.low >= FIRST_QUATER && encoder.high < THIRD_QUATER)
		{
			encoder.bits_to_follow++;
			encoder.high -= FIRST_QUATER;
			encoder.low -= FIRST_QUATER;
		}
		else
		{
			break;
		}

		encoder.low = 2 * encoder.low;
		encoder.high = 2 * encoder.high + 1;
	}

	return 0;
}

int Encode_stop()
{
	encoder.bits_to_follow++;
	if (encoder.low < FIRST_QUATER)
	{
		output_bits(0);
	} 
	else
	{
		output_bits(1);
	}
	printf("\n");
	return 0;
}
