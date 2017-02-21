#ifndef _RESIDUAL_H_
#define _RESIDUAL_H_

typedef struct Coeff4x4Block
{
	bool   emptyBlock;
	UINT16 coeffToken;
	UINT8  numCoeff;
	UINT8  trailingOnes;
	int  trailingSign[3];
	int    levels[16];
	UINT8  totalZeros;
	UINT8  runBefore[16];

	Coeff4x4Block()
	{
		emptyBlock = false;
		coeffToken = 0;
		numCoeff = 0;
		trailingOnes = 0;
		totalZeros = 0;
		memset(trailingSign, 0, 3 * sizeof(int));
		memset(levels, 0, 16 * sizeof(int));
		memset(runBefore, 0, 16 * sizeof(UINT8));
	}
} Coeff4x4Block;

class CMacroblock;
class CResidual
{
public:
	CResidual(UINT8 *pSODB, UINT32 offset, CMacroblock *mb);
	~CResidual();

	// ½âÎöºê¿éµÄÔ¤²â²Ð²î
	int Parse_macroblock_residual();
	UINT8 Get_sub_block_number_coeffs(int block_idc_x, int block_idc_y);

private:
	CMacroblock *m_macroblock_belongs;
	UINT8  *m_pSODB;
	UINT8  m_bypeOffset;
	UINT8  m_bitOffset;

	Coeff4x4Block luma_residual[4][4];
	Coeff4x4Block chroma_DC_residual[2];
	Coeff4x4Block chroma_AC_residual[2][2][2];

	int parse_luma_residual(UINT8 cbp_luma);
	int get_luma4x4_coeffs(int block_idc_x, int block_idc_y);

	int parse_chroma_residual(UINT8 cbp_chroma);
	int get_chroma_DC_coeffs(int chroma_idx);

	int get_numCoeff_and_trailingOnes(UINT8 &totalCoeff, UINT8 &trailingOnes, int &token, int numCoeff_vlcIdx);
	int get_coeff_level(int &level, int levelIdx, UINT8 trailingOnes, int suffixLength);
	int get_total_zeros(UINT8 &totalZeros, int totalZeros_vlcIdx);
	int get_run_before(UINT8 &runBefore, int runBefore_vlcIdx);

	int get_numCoeff_and_trailingOnes_chromaDC(UINT8 &totalCoeff, UINT8 &trailingOnes, int &token);

	int search_for_value_in_2D_table(UINT8 &value1, UINT8 &value2, int &code, int *lengthTable, int *codeTable, int tableWidth, int tableHeight);
};

#endif
