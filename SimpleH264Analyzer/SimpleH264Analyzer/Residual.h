#ifndef _RESIDUAL_H_
#define _RESIDUAL_H_

class CMacroblock;
class CResidual
{
public:
	CResidual(UINT8 *pSODB, UINT32 offset, CMacroblock *mb);
	~CResidual();

	// ½âÎöºê¿éµÄÔ¤²â²Ð²î
	int Parse_macroblock_residual();

private:
	CMacroblock *m_macroblock_belongs;
	UINT8  *m_pSODB;
	UINT8  m_bypeOffset;
	UINT8  m_bitOffset;
};

#endif
