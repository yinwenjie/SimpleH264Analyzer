#include "stdafx.h"
#include "Macroblock.h"
#include "SeqParamSet.h"
#include "PicParamSet.h"
#include "SliceHeader.h"
#include "SliceStruct.h"

#include "Residual.h"
#include "Macroblock_Defines.h"

using namespace std;

CMacroblock::CMacroblock(UINT8 *pSODB, UINT32 offset, int idx)
{
	m_pSODB = pSODB;
	m_bypeOffset = offset / 8;
	m_bitOffset = offset % 8;
	m_mbDataSize = offset;
	m_mb_idx = idx;
	m_transform_size_8x8_flag = false;

	m_intra16x16PredMode = -1;

	m_pps_active = NULL;
	m_slice = NULL;
	m_pred_struct = NULL;

	m_residual = NULL;

	m_coeffArray = NULL;

	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			m_pred_block[i][j] = 0;
		}
	}

	for (int i = 0; i < 16; i++)
	{
		m_intra_pred_mode[i] = -1;
	}
}


CMacroblock::~CMacroblock()
{
	if (m_pred_struct)
	{
		delete[] m_pred_struct;
		m_pred_struct = NULL;
	}

	if (m_coeffArray)
	{
		delete m_coeffArray;
		m_coeffArray = NULL;
	}

	if (m_residual)
	{
		delete m_residual;
		m_residual = NULL;
	}
}

void CMacroblock::Set_paramaters(CPicParamSet *pps)
{
	m_pps_active = pps;
}

void CMacroblock::Set_slice_struct(CSliceStruct *sliceStruct)
{
	m_slice = sliceStruct;
}

UINT32 CMacroblock::Parse_macroblock()
{
	UINT32 residualLength = 0;
	m_mb_type = Get_uev_code_num(m_pSODB, m_bypeOffset, m_bitOffset);

	if (m_mb_type == 25)
	{
		// To do: I-PCM mode...
	} 
	else if (m_mb_type == 0)
	{
		// Intra_NxN mode...
		if (m_pps_active->Get_transform_8x8_mode_flag())
		{
			m_transform_size_8x8_flag = Get_bit_at_position(m_pSODB, m_bypeOffset, m_bitOffset);
		}

		// Get prediction-block num...
		if (m_transform_size_8x8_flag)
		{
			// Using intra_8x8
			m_pred_struct = new IntraPredStruct[4];	
			for (int luma8x8BlkIdx = 0; luma8x8BlkIdx < 4; luma8x8BlkIdx++)
			{
				m_pred_struct[luma8x8BlkIdx].block_mode = 1;
				m_pred_struct[luma8x8BlkIdx].prev_intra_pred_mode_flag = Get_bit_at_position(m_pSODB, m_bypeOffset, m_bitOffset);
				if (!m_pred_struct[luma8x8BlkIdx].prev_intra_pred_mode_flag)
				{
					m_pred_struct[luma8x8BlkIdx].rem_intra_pred_mode = Get_uint_code_num(m_pSODB, m_bypeOffset, m_bitOffset, 3);
				}
			}
		} 
		else
		{
			// Using intra_4x4
			m_pred_struct = new IntraPredStruct[16];
			for (int luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; luma4x4BlkIdx++)
			{
				m_pred_struct[luma4x4BlkIdx].block_mode = 0;
				m_pred_struct[luma4x4BlkIdx].prev_intra_pred_mode_flag = Get_bit_at_position(m_pSODB, m_bypeOffset, m_bitOffset);
				if (!m_pred_struct[luma4x4BlkIdx].prev_intra_pred_mode_flag)
				{
					m_pred_struct[luma4x4BlkIdx].rem_intra_pred_mode = Get_uint_code_num(m_pSODB, m_bypeOffset, m_bitOffset, 3);
				}
			}
		}

		// intra_chroma_pred_mode
		m_intra_chroma_pred_mode = Get_uev_code_num(m_pSODB, m_bypeOffset, m_bitOffset);
	}
	else
	{
		// To do: Intra_16x16 mode
		m_intra16x16PredMode = (m_mb_type - 1) % 4;
		m_cbp_luma = (m_mb_type <= 12) ? 0 : 15;
		m_cbp_chroma = ((m_mb_type - 1) / 4) % 3;

		// intra_chroma_pred_mode
		m_intra_chroma_pred_mode = Get_uev_code_num(m_pSODB, m_bypeOffset, m_bitOffset);
	}

	if (m_mb_type == 0 || m_mb_type == 25)
	{
		m_coded_block_pattern = Get_me_code_num(m_pSODB, m_bypeOffset, m_bitOffset, 1);
		m_cbp_luma = m_coded_block_pattern % 16;
		m_cbp_chroma = m_coded_block_pattern / 16;
	}
	
	if (m_cbp_luma > 0 || m_cbp_chroma > 0 || (m_mb_type > 0 && m_mb_type < 25))
	{
		m_mb_qp_delta = Get_sev_code_num(m_pSODB, m_bypeOffset, m_bitOffset);
		m_mb_qp = m_pps_active->Get_pic_init_qp() + m_slice->m_sliceHeader->Get_slice_qp_delta() + m_mb_qp_delta;
	}

	// Êä³ömb headerÐÅÏ¢
	Dump_macroblock_info();

	if (m_cbp_luma > 0 || m_cbp_chroma > 0 || (m_mb_type > 0 && m_mb_type < 25))
	{
		interpret_mb_mode();
		m_residual = new CResidual(m_pSODB, m_bypeOffset * 8 + m_bitOffset, this);
		m_residual->Parse_macroblock_residual(residualLength);

		m_residual->Restore_coeff_matrix();
	}

	m_mbDataSize = m_bypeOffset * 8 + m_bitOffset - m_mbDataSize + residualLength;
	return m_mbDataSize;
}

void CMacroblock::Dump_macroblock_info()
{
#if TRACE_CONFIG_LOGOUT

#if TRACE_CONFIG_MACROBLOCK

	g_traceFile << "***** MB: " << to_string(m_mb_idx) << " *****" <<endl;
	g_traceFile << "mb_type: " << to_string(m_mb_type) << endl;
	if (m_mb_type == 0)
	{
		if (!m_transform_size_8x8_flag)
		{
			for (int idx = 0; idx < 16; idx++)
			{
				g_traceFile << "prev_intra4x4_pred_mode_flag: " << m_pred_struct[idx].prev_intra_pred_mode_flag << endl;
				if (!m_pred_struct[idx].prev_intra_pred_mode_flag)
				{
					g_traceFile << "rem_intra4x4_pred_mode: " << to_string(m_pred_struct[idx].rem_intra_pred_mode) << endl;
				}
			}
		}
	}

	g_traceFile << "intra_chroma_pred_mode: " << to_string(m_intra_chroma_pred_mode) << endl;
	if (m_mb_type == 0 || m_mb_type == 25)
	{
		g_traceFile << "coded_block_pattern: " << to_string(m_coded_block_pattern) << endl;
	}
	g_traceFile << "mb_qp_delta: " << to_string(m_mb_qp_delta) << endl;
#endif

#endif
}

int CMacroblock::Decode_macroblock()
{
	int err = 0;
	if (m_mb_type == I4MB)
	{
		err = get_pred_blocks_4x4();
		if (err < 0)
		{
			return err;
		}
	} 
	else if (m_mb_type == I16MB)
	{
	}

	return kPARSING_ERROR_NO_ERROR;
}

CPicParamSet * CMacroblock::Get_pps_active()
{
	return m_pps_active;
}

void CMacroblock::interpret_mb_mode()
{
	UINT8 slice_type = m_slice->m_sliceHeader->Get_slice_type();
	switch (slice_type)
	{
	case SLICE_TYPE_I:
		if (m_mb_type == 0)
		{
			m_mb_type = I4MB;
		}
		else if (m_mb_type == 25)
		{
			m_mb_type = IPCM;
		}
		else
		{
			m_mb_type = I16MB;
		}
		break;
	default:
		break;
	}
}

int CMacroblock::Get_number_current(int block_idc_x, int block_idc_y)
{
	int nC = 0, topIdx = 0, leftIdx = 0, leftNum = 0, topNum = 0;
	bool available_top = false, available_left = false;

	get_neighbor_available(available_top, available_left, topIdx, leftIdx, block_idc_x, block_idc_y);

	if (!available_left && !available_top)
	{
		nC = 0;
	}
	else
	{
		if (available_left)
		{			
			leftNum = get_left_neighbor_coeff_numbers(leftIdx, block_idc_x, block_idc_y);			
		}
		if (available_top)
		{		
			topNum = get_top_neighbor_coeff_numbers(topIdx, block_idc_x, block_idc_y);			
		}

		nC = leftNum + topNum;
		if (available_left && available_top)
		{
			nC++;
			nC >>= 1;
		}
	}
	
	
	return nC;
}

int CMacroblock::Get_number_current_chroma(int component, int block_idc_x, int block_idc_y)
{
	int nC = 0, topIdx = 0, leftIdx = 0, leftNum = 0, topNum = 0;
	bool available_top = false, available_left = false;

	get_neighbor_available(available_top, available_left, topIdx, leftIdx, block_idc_x, block_idc_y);

	if (!available_left && !available_top)
	{
		nC = 0;
	}
	else
	{
		if (available_left)
		{
			leftNum = get_left_neighbor_coeff_numbers_chroma(leftIdx, component, block_idc_x, block_idc_y);
		}
		if (available_top)
		{
			topNum = get_top_neighbor_coeff_numbers_chroma(topIdx, component, block_idc_x, block_idc_y);
		}

		nC = leftNum + topNum;
		if (available_left && available_top)
		{
			nC++;
			nC >>= 1;
		}
	}

	return nC;
}

int CMacroblock::get_neighbor_available(bool &available_top, bool &available_left, int &topIdx, int &leftIdx, int block_idc_x, int block_idc_y)
{
	int mb_idx = m_mb_idx;
	int width_in_mb = m_slice->m_sps_active->Get_pic_width_in_mbs();
	int height_in_mb = m_slice->m_sps_active->Get_pic_height_in_mbs();

	bool left_edge_mb = (mb_idx % width_in_mb == 0);
	bool top_edge_mb = (mb_idx < width_in_mb);
	if (!top_edge_mb)
	{
		available_top = true;
		topIdx = block_idc_y == 0 ? (mb_idx - width_in_mb) : mb_idx;
	}
	else //ÉÏ±ßÑØºê¿é
	{
		if (block_idc_y == 0)
		{
			available_top = false;
		} 
		else
		{
			available_top = true;
			topIdx = mb_idx;
		}
	}

	if (!left_edge_mb)
	{
		available_left = true;
		leftIdx = block_idc_x == 0 ? (mb_idx - 1) : mb_idx;
	}
	else //×ó±ßÑØºê¿é
	{
		if (block_idc_x == 0)
		{
			available_left = false;
		} 
		else
		{
			available_left = true;
			leftIdx = mb_idx;
		}
	}
	
	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_top_neighbor_coeff_numbers(int topIdx, int block_idc_x, int block_idc_y)
{
	int nzCoeff = 0, target_idx_y = 0;

	if (topIdx == m_mb_idx)
	{
		target_idx_y = block_idc_y - 1;
		nzCoeff = m_residual->Get_sub_block_number_coeffs(block_idc_x, target_idx_y);
	}
	else
	{
		const CMacroblock *targetMB = m_slice->Get_macroblock_at_index(topIdx);
		if (targetMB->m_residual)
		{
			nzCoeff = targetMB->m_residual->Get_sub_block_number_coeffs(block_idc_x, 3);
		}
		else
		{
			nzCoeff = 0;
		}
	}

	return nzCoeff;
}

int CMacroblock::get_left_neighbor_coeff_numbers(int leftIdx, int block_idc_x, int block_idc_y)
{
	int nzCoeff = 0, target_idx_x = 0;
	if (leftIdx == m_mb_idx)
	{
		target_idx_x = block_idc_x - 1;
		nzCoeff = m_residual->Get_sub_block_number_coeffs(target_idx_x, block_idc_y);
	}
	else
	{
		const CMacroblock *targetMB = m_slice->Get_macroblock_at_index(leftIdx);
		if (targetMB->m_residual)
		{
			nzCoeff = targetMB->m_residual->Get_sub_block_number_coeffs(3, block_idc_y);
		}
		else
		{
			nzCoeff = 0;
		}
	}

	return nzCoeff;
}

int CMacroblock::get_top_neighbor_coeff_numbers_chroma(int topIdx, int component, int block_idc_x, int block_idc_y)
{
	int nzCoeff = 0, target_idx_y = 0;
	if (topIdx == m_mb_idx)
	{
		target_idx_y = block_idc_y - 1;
		nzCoeff = m_residual->Get_sub_block_number_coeffs_chroma(component, block_idc_x, target_idx_y);
	}
	else
	{
		const CMacroblock *targetMB = m_slice->Get_macroblock_at_index(topIdx);
		if (targetMB->m_residual)
		{
			nzCoeff = targetMB->m_residual->Get_sub_block_number_coeffs_chroma(component, block_idc_x, 1);
		} 
		else
		{
			nzCoeff = 0;
		}		
	}
	return nzCoeff;
}

int CMacroblock::get_left_neighbor_coeff_numbers_chroma(int leftIdx, int component, int block_idc_x, int block_idc_y)
{
	int nzCoeff = 0, target_idx_x = 0;
	if (leftIdx == m_mb_idx)
	{
		target_idx_x = block_idc_x - 1;
		nzCoeff = m_residual->Get_sub_block_number_coeffs_chroma(component, target_idx_x, block_idc_y);
	}
	else
	{
		const CMacroblock *targetMB = m_slice->Get_macroblock_at_index(leftIdx);
		if (targetMB->m_residual)
		{
			nzCoeff = targetMB->m_residual->Get_sub_block_number_coeffs_chroma(component, 1, block_idc_y);
		}
		else
		{
			nzCoeff = 0;
		}
	}

	return nzCoeff;
}

int CMacroblock::search_for_value_in_2D_table(int &value1, int &value2, int &code, int *lengthTable, int *codeTable, int tableWidth, int tableHeight)
{
	int err = 0;
	int codeLen = 0;
	for (int yIdx = 0; yIdx < tableHeight; yIdx++)
	{
		for (int xIdx = 0; xIdx < tableWidth; xIdx++)
		{
			codeLen = lengthTable[xIdx];
			if (codeLen == 0)
			{
				continue;
			}
			code = codeTable[xIdx];
			if (Peek_uint_code_num(m_pSODB, m_bypeOffset, m_bitOffset, codeLen) == code)
			{
				value1 = xIdx;
				value2 = yIdx;
				m_bitOffset += codeLen;
				m_bypeOffset += (m_bitOffset / 8);
				m_bitOffset %= 8;
				err = 0;
				goto found;
			}
		}
		lengthTable += tableWidth;
		codeTable += tableWidth;
	}
	err = kPARSING_CAVLC_CODE_NOT_FOUND;

found:
	return err;
}

int CMacroblock::get_pred_blocks_4x4()
{	
	int err = 0;
	for (UINT8 block_idx = 0; block_idx < 16; block_idx++)
	{
		err = get_pred_block_of_idx(block_idx);
		if (err < 0)
		{
			return err;
		}
	}

	return kPARSING_ERROR_NO_ERROR;
}

/*
block position of each index:
	row:  0   1   2   3
		 _______________
col:0	| 0 | 1 | 4 | 5 |
	1	| 2 | 3 | 6 | 7 |
	2	| 8 | 9 | 12| 13|
	3	| 10| 11| 14| 15|
*/
int CMacroblock::block_index_to_position(UINT8 blkIdx, UINT8 &block_pos_row, UINT8 &block_pos_column)
{
/*
block8_index of each index:			block4_index of each index:
	row:  0   1   2   3					row:  0   1   2   3
		 _______________					 _______________
col:0	| 0 | 0 | 1 | 1 |			col:0	| 0 | 1 | 0 | 1 |
	1	| 0 | 0 | 1 | 1 |				1	| 2 | 3 | 2 | 3 |
	2	| 2 | 2 | 3 | 3 |				2	| 0 | 1 | 0 | 1 |
	3	| 2 | 2 | 3 | 3 |				3	| 2 | 3 | 2 | 3 |
*/
	UINT8 block8_idx = blkIdx / 4, block4_index = blkIdx % 4; /* 0 1 2 3 */

/*
(block_row, block_column) of each index:
	row:    0       1       2       3
		 _______________ _______________
col:0	| (0,0) | (1,0) | (0,0) | (1,0) |
	1	| (0,1) | (1,1) | (0,1) | (1,1) |
	2	| (0,0) | (1,0) | (0,0) | (1,0) |
	3	| (0,1) | (1,1) | (0,1) | (1,1) |
*/
	UINT8 block4_row = block4_index % 2, block4_column = block4_index / 2; /* 0 1 */

/*
(block_row, block_column) of each index:
	row:    0       1       2       3
		 _______________ _______________
col:0	| (0,0) | (1,0) | (2,0) | (3,0) |
	1	| (0,1) | (1,1) | (2,1) | (3,1) |
	2	| (0,2) | (1,2) | (2,2) | (3,2) |
	3	| (0,3) | (1,3) | (2,3) | (3,3) |
*/
	UINT8 block_row = block4_row + 2 * (block8_idx % 2), block_column = block4_column + 2 * (block8_idx / 2);

	block_pos_row = block_row;
	block_pos_column = block_column;

	return kPARSING_ERROR_NO_ERROR;
}

UINT8 CMacroblock::position_to_block_index(UINT8 block_pos_row, UINT8 block_pos_column)
{
	int block8_row = block_pos_row / 2, block8_column = block_pos_column / 2, block8_index = block8_row + block8_column * 2;
	int block4_row = block_pos_row % 2, block4_column = block_pos_column % 2, block4_index = block4_row + block4_column * 2;

	return block4_index + 4 * block8_index;
}

int CMacroblock::get_pred_block_of_idx(UINT8 blkIdx)
{
	UINT8 topMBType = 0, leftMBType = 0;
	NeighborBlocks neighbors;
	UINT8 blk_row = 0, blk_column = 0;

// 	for (int idx = 0; idx < 16; idx++)
// 	{
// 		block_index_to_position(idx, blk_row, blk_column);
// 		printf("%d: (%d, %d)\n", idx, blk_row, blk_column);
// 	}

	block_index_to_position(blkIdx, blk_row, blk_column);
	
	get_neighbor_blocks_availablility(neighbors, blk_row, blk_column);

	bool dcPredModePredictionFlag = false;
	bool available_left = neighbors.flags & 1, available_top = neighbors.flags & 2, available_top_right = neighbors.flags & 4, available_top_left = neighbors.flags & 8;
	int leftMode = -1, topMode = -1, this_intra_mode = -1;

	if (!available_left || !available_top)
	{
		dcPredModePredictionFlag = true;
	}
	else
	{
		CMacroblock *leftMB = m_slice->Get_macroblock_at_index(neighbors.left.target_mb_idx);
		CMacroblock *topMB = m_slice->Get_macroblock_at_index(neighbors.top.target_mb_idx);
		leftMBType = leftMB->m_mb_type;
		topMBType = topMB->m_mb_type;
		leftMode = leftMB->get_neighbor_block_intra_mode(neighbors.left);
		topMode = topMB->get_neighbor_block_intra_mode(neighbors.top);
	}

	if (dcPredModePredictionFlag || leftMBType != I4MB)
	{
		leftMode = 2;
	} 
	if (dcPredModePredictionFlag || topMBType != I4MB)
	{
		topMode = 2;
	}

	int predIntra4x4PredMode = min(leftMode, topMode);
	if (m_pred_struct[blkIdx].prev_intra_pred_mode_flag)
	{
		this_intra_mode = predIntra4x4PredMode;
	} 
	else if (m_pred_struct[blkIdx].rem_intra_pred_mode < predIntra4x4PredMode)
	{
		this_intra_mode = m_pred_struct[blkIdx].rem_intra_pred_mode;
	} 
	else
	{
		this_intra_mode = m_pred_struct[blkIdx].rem_intra_pred_mode + 1;
	}

	m_intra_pred_mode[blkIdx] = this_intra_mode;
	m_pred_struct[blkIdx].block_mode = this_intra_mode;

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::construct_pred_block(NeighborBlocks neighbors, UINT8 blkIdx, int predMode)
{
	UINT8 refPixBuf[13] = { 0 };

	get_reference_pixels(neighbors, blkIdx, refPixBuf);

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_reference_pixels(NeighborBlocks neighbors, UINT8 blkIdx, UINT8 *refPixBuf)
{

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_pred_mode_at_idx(UINT8 blkIdx)
{
	return m_intra_pred_mode[blkIdx];
}

int CMacroblock::get_neighbor_blocks_availablility(NeighborBlocks &neighbors, int block_idc_row, int block_idc_column)
{
	int mb_idx = m_mb_idx;
	int width_in_mb = m_slice->m_sps_active->Get_pic_width_in_mbs();
	int height_in_mb = m_slice->m_sps_active->Get_pic_height_in_mbs();

	bool left_edge_mb = (mb_idx % width_in_mb == 0);
	bool top_edge_mb = (mb_idx < width_in_mb);
	bool right_edge_mb = (mb_idx + 1 % width_in_mb == 0);

	if (!left_edge_mb)
	{
		neighbors.flags |= 1;
		neighbors.left.target_mb_idx = (block_idc_row == 0 ? (mb_idx - 1) : mb_idx);
		neighbors.left.block_row = (block_idc_row == 0 ? 3 : (block_idc_row - 1));
		neighbors.left.block_column = block_idc_column;
	}
	else //×ó±ßÑØºê¿é
	{
		if (block_idc_row == 0)
		{
			neighbors.flags &= 14;
		}
		else //ÄÚ²¿¿é
		{
			neighbors.flags |= 1;
			neighbors.left.target_mb_idx = mb_idx;
			neighbors.left.block_row = block_idc_row - 1;
			neighbors.left.block_column = block_idc_column;
		}
	}

	if (!top_edge_mb)
	{
		neighbors.flags |= 2;
		neighbors.top.target_mb_idx = (block_idc_column == 0 ? mb_idx - width_in_mb : mb_idx);
		neighbors.top.block_row = block_idc_row;
		neighbors.top.block_column = (block_idc_column == 0) ? 3 : block_idc_column - 1;
	}
	else //ÉÏ±ßÑØºê¿é
	{
		if (block_idc_column == 0)
		{
			neighbors.flags &= 13;
		}
		else //ÄÚ²¿¿é
		{
			neighbors.flags |= 2;
			neighbors.top.target_mb_idx = mb_idx;
			neighbors.top.block_row = block_idc_row;
			neighbors.top.block_column = block_idc_column - 1;
		}
	}

	if ((left_edge_mb && block_idc_row == 0) || (top_edge_mb && block_idc_column == 0))
	{
		neighbors.flags &= 7;
	}
	else
	{
		neighbors.flags |= 8;
		if (block_idc_row != 0 && block_idc_column != 0)
		{
			neighbors.top_left.target_mb_idx = mb_idx;
			neighbors.top_left.block_row = block_idc_row - 1;
			neighbors.top_left.block_column = block_idc_column - 1;
		}
		else if (block_idc_row == 0 && block_idc_column == 0)
		{
			neighbors.top_left.target_mb_idx = mb_idx - width_in_mb - 1;
			neighbors.top_left.block_row = 3;
			neighbors.top_left.block_column = 3;
		}
		else if (block_idc_row != 0 && block_idc_column == 0)
		{
			neighbors.top_left.target_mb_idx = mb_idx - width_in_mb;
			neighbors.top_left.block_row = block_idc_row - 1;
			neighbors.top_left.block_column = 3;
		}
		else if (block_idc_row == 0 && block_idc_column != 0)
		{
			neighbors.top_left.target_mb_idx = mb_idx - 1;
			neighbors.top_left.block_row = 3;
			neighbors.top_left.block_column = block_idc_column - 1;
		}
	}

	if ((right_edge_mb && block_idc_row == 3) || (top_edge_mb && block_idc_column == 0) || (block_idc_row == 3 && block_idc_column != 0))
	{
		neighbors.flags &= 11;
	}
	else
	{
		neighbors.flags |= 4;
		if (block_idc_row == 3 && block_idc_column == 0)
		{
			neighbors.top_right.target_mb_idx = mb_idx - width_in_mb;
			neighbors.top_right.block_row = 0;
			neighbors.top_right.block_column = 3;
		}
		else if (block_idc_column == 0 && block_idc_row != 3)
		{
			neighbors.top_right.target_mb_idx = mb_idx - width_in_mb - 1;
			neighbors.top_right.block_row = block_idc_row + 1;
			neighbors.top_right.block_column = 3;
		}
		else if (block_idc_row != 3 && block_idc_column != 0)
		{
			neighbors.top_right.target_mb_idx = mb_idx;
			neighbors.top_right.block_row = block_idc_row + 1;
			neighbors.top_right.block_column = block_idc_column - 1;
		}
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_neighbor_block_intra_mode(NeighborBlockPos block)
{
	UINT8 block_row = block.block_row, block_column = block.block_column;
	UINT8 block_index = position_to_block_index(block_row, block_column);

	return m_intra_pred_mode[block_index];
}