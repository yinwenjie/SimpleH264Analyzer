#ifndef _MACROBLOCK_H_
#define _MACROBLOCK_H_

class CPicParamSet;
class CSliceHeader;

// 宏块类型
typedef enum
{
	P8x8 = 8,
	I4MB,
	I16MB,
	IBLOCK,
	SI4MB,
	MAXMODE,
	IPCM
} MacroblockType;

// 预测模式结构
typedef struct IntraPredStruct
{
	UINT8 block_mode;	//block_mode : 0 - 4x4 mode; 1 - 8x8 mode;
	bool prev_intra_pred_mode_flag;
	UINT8 rem_intra_pred_mode;

	IntraPredStruct()
	{
		block_mode = 0;
		prev_intra_pred_mode_flag = false;
		rem_intra_pred_mode = 0;
	}
} IntraPredStruct;

// 变换系数矩阵
typedef struct MacroBlockCoeffArray
{
	int luma_coeff[4][4][16];
	MacroBlockCoeffArray()
	{
		memset(luma_coeff, 0, 256 * sizeof(int));
	}
} MacroBlockCoeffArray;

// 宏块类
class CMacroblock
{
public:
	CMacroblock(UINT8 *pSODB, UINT32 offset, int idx);
	virtual ~CMacroblock();

	MacroBlockCoeffArray *m_coeffArray;

	void Set_paramaters(CPicParamSet *pps);
	void Set_slice_header(CSliceHeader *sliceHeader);
	UINT32 Parse_macroblock();
	void Dump_macroblock_info();

	UINT8  m_mb_type;

private:
	UINT8  *m_pSODB;
	UINT32 m_mbDataSize;
	UINT8  m_bypeOffset;
	UINT8  m_bitOffset;

	CPicParamSet *m_pps_active;
	CSliceHeader *m_slice_header;

	int    m_mb_idx;
	bool   m_transform_size_8x8_flag;

	IntraPredStruct *m_pred_struct;
	UINT8  m_intra_chroma_pred_mode;

	UINT8  m_coded_block_pattern;
	UINT8  m_mb_qp_delta;

	UINT8  m_cbp_luma;
	UINT8  m_cbp_chroma;

	void interpret_mb_mode();

	int get_luma_coeffs();
	int get_luma4x4_coeffs(int block_idc_x, int block_idc_y);
};

#endif
