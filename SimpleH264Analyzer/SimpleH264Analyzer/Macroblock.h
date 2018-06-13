#ifndef _MACROBLOCK_H_
#define _MACROBLOCK_H_

class CPicParamSet;
class CSliceStruct;
class CResidual;

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

//4×4帧内预测模式
typedef enum
{
	VERT_PRED,
	HOR_PRED,
	DC_PRED,
	DIAG_DOWN_LEFT_PRED,
	DIAG_DOWN_RIGHT_PRED,
	VERT_RIGHT_PRED,
	HOR_DOWN_PRED,
	VERT_LEFT_PRED,
	HOR_UP_PRED
} IntraMode4x4;

//16×16帧内预测模式
typedef enum
{
	VERT_PRED_16,
	HOR_PRED_16,
	DC_PRED_16,
	PLANE_16
} IntraMode16x16;

// 预测模式结构
typedef struct Intrleft_diff2_negativeredStruct
{
	UINT8 block_mode;	//block_mode : 0 - 4x4 mode; 1 - 8x8 mode;
	bool prev_intra_pred_mode_flag;
	UINT8 rem_intra_pred_mode;

	Intrleft_diff2_negativeredStruct()
	{
		block_mode = 0;
		prev_intra_pred_mode_flag = false;
		rem_intra_pred_mode = 0;
	}
} Intrleft_diff2_negativeredStruct;

// 变换系数矩阵
typedef struct MacroBlockCoeffArray
{
	int luma_coeff[4][4][16];
	MacroBlockCoeffArray()
	{
		memset(luma_coeff, 0, 256 * sizeof(int));
	}
} MacroBlockCoeffArray;

//相邻块位置结构
typedef struct NeighborBlockPos
{
	UINT32 target_mb_idx;
	UINT8 block_row;
	UINT8 block_column;
} NeighborBlockPos;

//相邻块信息
typedef struct NeighborBlocks
{
	UINT8 flags;				//Availability of neighbor blocks: 1 - left; 2 - top; 4 - top_right; 8 - top_left;
	NeighborBlockPos left;
	NeighborBlockPos top;
	NeighborBlockPos top_right;
	NeighborBlockPos top_left;
} NeighborBlocks;

// 宏块类
class CMacroblock
{
public:
	CMacroblock(UINT8 *pSODB, UINT32 offset, int idx);
	virtual ~CMacroblock();

	MacroBlockCoeffArray *m_coeffArray;

	void Set_paramaters(CPicParamSet *pps);
	void Set_slice_struct(CSliceStruct *sliceStruct);
	UINT32 Parse_macroblock();
	void Dump_macroblock_info();
	int Decode_macroblock();
	int Deblock_macroblock();

	CPicParamSet *Get_pps_active();

	int Get_number_current(int block_idc_row, int block_idc_column);
	int Get_number_current_chroma(int component, int block_idc_row, int block_idc_column);

	UINT8  m_mb_type;
	UINT8  m_cbp_luma;
	UINT8  m_cbp_chroma;
	int  m_mb_qp;
	int  m_intra16x16PredMode;

private:
	UINT8  *m_pSODB;
	UINT32 m_mbDataSize;
	UINT32  m_bypeOffset;
	UINT8  m_bitOffset;

	CPicParamSet *m_pps_active;
	CSliceStruct *m_slice;

	int    m_mb_idx;
	bool   m_transform_size_8x8_flag;

	Intrleft_diff2_negativeredStruct *m_pred_struct;
	UINT8  m_intra_chroma_pred_mode;

	UINT8  m_coded_block_pattern;
	UINT8  m_mb_qp_delta;

	UINT8 m_intra_pred_mode[16];
	CResidual *m_residual;
	UINT8 m_pred_block[16][4][4];
	UINT8 m_reconstructed_block[4][4][4][4];

	int    m_mb_alpha_c0_offset;
	int    m_mb_beta_offset;

	void interpret_mb_mode();

	int get_neighbor_available(bool &available_top, bool &available_left, int &topIdx, int &leftIdx, int block_idc_row, int block_idc_column);
	int get_top_neighbor_coeff_numbers(int topIdx, int block_idc_row, int block_idc_column);
	int get_left_neighbor_coeff_numbers(int leftIdx, int block_idc_row, int block_idc_column);
	int get_top_neighbor_coeff_numbers_chroma(int topIdx, int component, int block_idc_row, int block_idc_column);
	int get_left_neighbor_coeff_numbers_chroma(int leftIdx, int component, int block_idc_row, int block_idc_column);

	int search_for_value_in_2D_table(int &value1, int &value2, int &code, int *lengthTable, int *codeTable, int tableWidth, int tableHeight);

	int get_intra_blocks_16x16();
	int get_pred_block_16(NeighborBlocks &neighbors, UINT8 &up_left, UINT8 up[16], UINT8 left[16]);
	int get_neighbor_mb_availablility(NeighborBlocks &neighbors);
	int get_reference_pixels_16(const NeighborBlocks &neighbors, UINT8 &up_left, UINT8 up[16], UINT8 left[16]);
	int reconstruct_block_16();
	void dump_block16_info();

	int get_intra_blocks_4x4();

	int get_pred_block_of_idx(UINT8 blkIdx);
	int reconstruct_block_of_idx(UINT8 block_idx);

	int construct_pred_block(NeighborBlocks neighbors, UINT8 blkIdx, int predMode);
	int get_reference_pixels(NeighborBlocks neighbors, UINT8 blkIdx, UINT8 *refPixBuf);
	int get_pred_mode_at_idx(UINT8 blkIdx);
	void dump_block_info(UINT8 blkIdx);

	int get_neighbor_blocks_availablility(NeighborBlocks &neighbors, int block_idc_row, int block_idc_column);
	int get_neighbor_block_intra_mode(NeighborBlockPos block);

	// Deblocking methods
	int get_filtering_strength(int edge, int strength[16]);
	int filter_block_edge(int dir, int edge, int strength[16], int component);
	int get_edge_pixel_item(int dir, int target_mb_idx, int edge, int pix_idx, int luma, int pixel_arr[8]);
	int filter_pixel(int *pix_vals, int alpha_val, int beta_val, int *clip_table, int strength[16], int pixel_arr[8], int strength_idx, int component);
	int set_edge_pixel_item(int *pix_vals, int dir, int target_mb_idx, int edge, int pix_idx, int luma, int pixel_arr[8]);
};

#endif
