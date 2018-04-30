#ifndef _RESIDUAL_H_
#define _RESIDUAL_H_

const int SNGL_SCAN[16][2] =
{
	{ 0, 0 },{ 1, 0 },{ 0, 1 },{ 0, 2 },
	{ 1, 1 },{ 2, 0 },{ 3, 0 },{ 2, 1 },
	{ 1, 2 },{ 0, 3 },{ 1, 3 },{ 2, 2 },
	{ 3, 1 },{ 3, 2 },{ 2, 3 },{ 3, 3 }
};

//! Dequantization coefficients
const int dequant_coef[6][4][4] = {
	{ { 10, 13, 10, 13 },{ 13, 16, 13, 16 },{ 10, 13, 10, 13 },{ 13, 16, 13, 16 } },
	{ { 11, 14, 11, 14 },{ 14, 18, 14, 18 },{ 11, 14, 11, 14 },{ 14, 18, 14, 18 } },
	{ { 13, 16, 13, 16 },{ 16, 20, 16, 20 },{ 13, 16, 13, 16 },{ 16, 20, 16, 20 } },
	{ { 14, 18, 14, 18 },{ 18, 23, 18, 23 },{ 14, 18, 14, 18 },{ 18, 23, 18, 23 } },
	{ { 16, 20, 16, 20 },{ 20, 25, 20, 25 },{ 16, 20, 16, 20 },{ 20, 25, 20, 25 } },
	{ { 18, 23, 18, 23 },{ 23, 29, 23, 29 },{ 18, 23, 18, 23 },{ 23, 29, 23, 29 } }
};

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
	int Parse_macroblock_residual(UINT32 &dataLength);
	UINT8 Get_sub_block_number_coeffs(int block_idc_row, int block_idc_column);
	UINT8 Get_sub_block_number_coeffs_chroma(int component, int block_idc_row, int block_idc_column);
	void Restore_coeff_matrix();

	void  Dump_coeff_block(int blk_idx);
	void  Dump_residual_luma(int blockType);
	void  Dump_residual_chroma(UINT8 cbp_chroma);
	void  Dump_residual_chroma_DC();
	void  Dump_residual_chroma_AC();
	void  Dump_residual_luma16x16_DC();

	int m_residual_matrix_luma[16][4][4];
	int m_residual_matrix_chroma[2][4][4][4];

private:
	CMacroblock *m_macroblock_belongs;
	UINT8  *m_pSODB;
	UINT32 m_bypeOffset;
	UINT8  m_bitOffset;

	int m_qp;

	int m_coeff_matrix_luma[16][4][4];
	int m_coeff_matrix_chroma[2][4][4][4];

	Coeff4x4Block luma_residual[4][4];
	Coeff4x4Block chroma_DC_residual[2];
	Coeff4x4Block chroma_AC_residual[2][2][2];

	Coeff4x4Block luma_residual16x16_DC;
	Coeff4x4Block luma_residual16x16_AC[4][4];

	int parse_luma_residual(int blockType, UINT8 cbp_luma);
	int get_luma4x4_coeffs(int blockType, int block_idc_row, int block_idc_column);

	int parse_luma_residual_16x16_DC();

	int parse_chroma_residual(UINT8 cbp_chroma);
	int get_chroma_DC_coeffs(int chroma_idx);
	int get_chroma_AC_coeffs(int chroma_idx, int block_idc_row, int block_idc_column);

	int get_numCoeff_and_trailingOnes(UINT8 &totalCoeff, UINT8 &trailingOnes, int &token, int numCoeff_vlcIdx);
	int get_coeff_level(int &level, int levelIdx, UINT8 trailingOnes, int suffixLength);
	int get_total_zeros(UINT8 &totalZeros, int totalZeros_vlcIdx);
	int get_run_before(UINT8 &runBefore, int runBefore_vlcIdx);

	int get_numCoeff_and_trailingOnes_chromaDC(UINT8 &totalCoeff, UINT8 &trailingOnes, int &token);
	int get_total_zeros_chromaDC(UINT8 &totalZeros, int totalZeros_vlcIdx);

	int search_for_value_in_2D_table(UINT8 &value1, UINT8 &value2, int &code, int *lengthTable, int *codeTable, int tableWidth, int tableHeight);

	void restore_8x8_coeff_block_luma(int (*matrix)[4][4], int idx, int blockType);
	void restore_4x4_coeff_block_luma(int column_idx, int row_idx, int blockType);
	void restore_8x8_coeff_block_chroma_DC(int(*matrix)[4][4][4], int idx);
	void restore_8x8_coeff_block_chroma_AC(int(*matrix)[4][4][4], int idx);
	void restore_16x16_coeff_block_luma_DC(int(*matrix)[4][4]);

	void insert_matrix(int(*matrix)[4][4], int *block, int start, int maxCoeffNum, int x, int y);
	void coeff_invers_transform(int(*coeff_buf)[4], int(*residual_buf)[4]);
	void coeff_invers_DC_coeff(int(*coeff_buf)[4]);
};

#endif
