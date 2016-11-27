#include "stdafx.h"
#include "Macroblock.h"


CMacroblock::CMacroblock(UINT8 *pSODB, UINT32 offset)
{
	memset(this, 0, sizeof(CMacroblock));

	m_pSODB = pSODB + offset;
}


CMacroblock::~CMacroblock()
{
}

UINT32 CMacroblock::Parse_macroblock()
{
	UINT32 macroblockLength = 0;
	UINT8 bytePosition = 0, bitPosition = 0;

	m_mb_type = Get_uev_code_num(m_pSODB, bytePosition, bitPosition);

	return macroblockLength;
}
