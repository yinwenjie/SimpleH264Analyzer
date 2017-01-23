#include "stdafx.h"
#include "CAVLC.h"
#include "CAVLC_Map.h"
#include <math.h>
#include <algorithm>
#include <iostream>

using namespace std;

int get_total_coeffs(const int coeff[16])
{
	int ret = 0;
	for (int idx = 0; idx < 16; idx++)
	{
		if (coeff[idx])
		{
			ret++;
		}
	}

	return ret;
}

int get_trailing_ones(const int coeff[16], int trailingSign[3])
{
	int ret = 0;
	for (int idx = 15; idx >= 0; idx--)
	{
		if (abs(coeff[idx]) > 1 || ret == 3)
		{
			break;
		} 
		else if (abs(coeff[idx]) == 1)
		{
			if (coeff[idx] == 1)
			{
				trailingSign[ret] = 1;
			} 
			else
			{
				trailingSign[ret] = -1;
			}
			ret++;
		}
	}

	return ret;
}

int get_levels(const int coeff[16], int levels[], int levelCnt)
{
	int levelIdx = levelCnt - 1;
	for (int idx = 0; idx < 16; idx++)
	{
		if (coeff[idx] != 0)
		{
			levels[levelIdx--] = coeff[idx];
			if (levelIdx < 0)
			{
				break;
			}
		}
	}
	return 0;
}

int get_totalzeros_runbefore(const int coeff[16], int *runBefore, int *zerosLeft, int totalCoeffs)
{
	int totalZeros = 0, idx = 15, runIdx = 0;
	for (; idx >= 0; idx--)
	{
		if (coeff[idx])
		{
			break;
		}
	}
	totalZeros = idx - totalCoeffs + 1;

	for (; idx >= 0; idx--)
	{
		if (coeff[idx] == 0)
		{
			continue;
		}

		for (int run = 0; run <= idx; run++)
		{
			if (coeff[idx - 1 - run] == 0)
			{
				runBefore[runIdx]++;
				zerosLeft[runIdx]++;
			} 
			else
			{
				runIdx++;
				break;
			}
		}
	}

	for (int a = 0; a < runIdx; a++)
	{
		for (int b = a + 1; b < runIdx; b++)
		{
			zerosLeft[a] += zerosLeft[b];
		}
	}

	return totalZeros;
}

string encode_levels(int level, int &suffixLength)
{
	string retStr;
	
	int levelCode = 0;
	if (level > 0)
	{
		levelCode = (level << 1) - 2;
	} 
	else
	{
		levelCode = -(level << 1) - 1;
	}

	int levelPrefix = levelCode / (1 << suffixLength);
	int levelSuffix = levelCode % (1 << suffixLength);

	for (int idx = 0; idx < levelPrefix; idx++)
	{
		retStr += "0";
	}
	retStr += "1";

	for (int idx = 0; idx < suffixLength; idx++)
	{
		if ((levelSuffix >> (suffixLength - idx - 1) & 1) == 1)
		{
			retStr += "1";
		} 
		else
		{
			retStr += "0";
		}
	}

	return retStr;
}


string Encoding_cavlc_16x16(const int coeff[16])
{
	string cavlcCode;

	int trailingSign[3] = { 0 };
	int totalCoeffs = get_total_coeffs(coeff);
	int trailingOnes = get_trailing_ones(coeff, trailingSign);

	int levelCnt = totalCoeffs - trailingOnes;
	int *levels = new int[levelCnt];
	get_levels(coeff, levels, levelCnt);

	int *runBefore = new int[totalCoeffs];
	int *zerosLeft = new int[totalCoeffs];
	memset(runBefore, 0, totalCoeffs * sizeof(int));
	memset(zerosLeft, 0, totalCoeffs * sizeof(int));
	int totalZeros = get_totalzeros_runbefore(coeff, runBefore, zerosLeft,totalCoeffs);
	
	// coeff_token
	if (totalCoeffs >= 3)
	{
		cavlcCode += coeffTokenMap[0][(totalCoeffs - 3) * 4 + trailingOnes + 6];
	}
	else if (totalCoeffs <= 1)
	{
		cavlcCode += coeffTokenMap[0][totalCoeffs + trailingOnes];
	}
	else if (totalCoeffs == 2)
	{
		cavlcCode += coeffTokenMap[0][totalCoeffs + trailingOnes + 1];
	}
	// trailing_sign
	for (int idx = 0; idx < trailingOnes; idx++)
	{
		if (trailingSign[idx] == 1)
		{
			cavlcCode += "0";
		}
		else if (trailingSign[idx] == -1)
		{
			cavlcCode += "1";
		}
	}

	// levels
	int suffixLength = 0;
	if (totalCoeffs > 10 && trailingOnes < 3)
	{
		suffixLength = 1;
	}
	for (int idx = 0; idx < levelCnt; idx++)
	{
		cavlcCode += encode_levels(levels[idx], suffixLength);
		if ((abs(levels[idx]) > (suffixLength == 0 ? 0 : (3 << (suffixLength - 1)))) && suffixLength < 6)
		{
			suffixLength++;
		}
	}

	// totalZeros
	cavlcCode += totalZerosMap[totalZeros][totalCoeffs];

	// runBefore
	for (int idx = 0; idx < totalCoeffs - 1; idx++)
	{
		if (zerosLeft[idx] == 0)
		{
			break;
		}
		cavlcCode += runBeforeMap[runBefore[idx]][zerosLeft[idx]];
	}

	delete[] levels;
	delete[] runBefore;
	delete[] zerosLeft;
	return cavlcCode;
}
