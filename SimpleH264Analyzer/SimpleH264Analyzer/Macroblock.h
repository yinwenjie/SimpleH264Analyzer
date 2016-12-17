#ifndef _MACROBLOCK_H_
#define _MACROBLOCK_H_

class CPicParamSet;

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

class CMacroblock
{
public:
	CMacroblock(UINT8 *pSODB, UINT32 offset);
	virtual ~CMacroblock();

	void Set_paramaters(CPicParamSet *pps);
	UINT32 Parse_macroblock();

private:
	UINT8  *m_pSODB;
	UINT8  m_bypeOffset;
	UINT8  m_bitOffset;

	CPicParamSet *m_pps_active;

	UINT8  m_mb_type;
	bool   m_transform_size_8x8_flag;

	IntraPredStruct *m_pred_struct;
	UINT8  m_intra_chroma_pred_mode;

	UINT8  m_coded_block_pattern;
	UINT8  m_mb_qp_delta;

	UINT8  m_cbp_luma;
	UINT8  m_cbp_chroma;
};

#endif
