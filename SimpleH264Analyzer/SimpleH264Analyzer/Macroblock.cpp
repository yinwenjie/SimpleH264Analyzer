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

	for (int idx = 0; idx < 16; idx++)
	{
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				m_pred_block[idx][i][j] = 0;
			}
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
			m_pred_struct = new Intrleft_diff2_negativeredStruct[4];	
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
			m_pred_struct = new Intrleft_diff2_negativeredStruct[16];
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
	interpret_mb_mode();
	if (m_cbp_luma > 0 || m_cbp_chroma > 0 || (m_mb_type > 0 && m_mb_type < 25))
	{
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
		err = get_intra_blocks_4x4();
		if (err < 0)
		{
			return err;
		}
	} 
	else if (m_mb_type == I16MB)
	{
		err = get_intra_blocks_16x16();
		if (err < 0)
		{
			return err;
		}
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::Deblock_macroblock()
{
	int width_in_mb = m_slice->m_sps_active->Get_pic_width_in_mbs();
	int height_in_mb = m_slice->m_sps_active->Get_pic_height_in_mbs();
	int total_filter_strength = 0, filter_strength_arr[16] = { 0 };

	bool filterLeftMbEdgeFlag = (m_mb_idx % width_in_mb != 0);
	bool filterTopMbEdgeFlag = (m_mb_idx >= width_in_mb);

	if (m_slice->m_sliceHeader->m_disable_deblocking_filter_idc == 1)
	{
		return kPARSING_ERROR_NO_ERROR;
	}

	m_mb_alpha_c0_offset = m_slice->m_sliceHeader->m_slice_alpha_c0_offset;
	m_mb_beta_offset = m_slice->m_sliceHeader->m_slice_beta_offset;

	// Vertical filtering luma
	for (int ver_edge = 0; ver_edge < 4; ver_edge++)
	{
		bool left_frame_edge = (ver_edge || filterLeftMbEdgeFlag);
		if (ver_edge || left_frame_edge)
		{
			total_filter_strength = get_filtering_strength(ver_edge, filter_strength_arr);
			if (total_filter_strength)
			{
				filter_block_edge(0, ver_edge, filter_strength_arr, 0);
			}
		}
	}

	// Horizontal filtering luma
	for (int hor_edge = 0; hor_edge < 4; hor_edge++)
	{
		bool top_frame_edge = (hor_edge && filterTopMbEdgeFlag);
		if (hor_edge || top_frame_edge)
		{
			total_filter_strength = get_filtering_strength(hor_edge, filter_strength_arr);
			if (total_filter_strength)
			{
				filter_block_edge(1, hor_edge, filter_strength_arr, 0);
			}
		}
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

int CMacroblock::Get_number_current(int block_idc_row, int block_idc_column)
{
	int nC = 0, topIdx = 0, leftIdx = 0, leftNum = 0, topNum = 0;
	bool available_top = false, available_left = false;

	get_neighbor_available(available_top, available_left, topIdx, leftIdx, block_idc_row, block_idc_column);

	if (!available_left && !available_top)
	{
		nC = 0;
	}
	else
	{
		if (available_left)
		{			
			leftNum = get_left_neighbor_coeff_numbers(leftIdx, block_idc_row, block_idc_column);			
		}
		if (available_top)
		{		
			topNum = get_top_neighbor_coeff_numbers(topIdx, block_idc_row, block_idc_column);			
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

int CMacroblock::Get_number_current_chroma(int component, int block_idc_row, int block_idc_column)
{
	int nC = 0, topIdx = 0, leftIdx = 0, leftNum = 0, topNum = 0;
	bool available_top = false, available_left = false;

	get_neighbor_available(available_top, available_left, topIdx, leftIdx, block_idc_row, block_idc_column);

	if (!available_left && !available_top)
	{
		nC = 0;
	}
	else
	{
		if (available_left)
		{
			leftNum = get_left_neighbor_coeff_numbers_chroma(leftIdx, component, block_idc_row, block_idc_column);
		}
		if (available_top)
		{
			topNum = get_top_neighbor_coeff_numbers_chroma(topIdx, component, block_idc_row, block_idc_column);
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

int CMacroblock::get_neighbor_available(bool &available_top, bool &available_left, int &topIdx, int &leftIdx, int block_idc_row, int block_idc_column)
{
	int mb_idx = m_mb_idx;
	int width_in_mb = m_slice->m_sps_active->Get_pic_width_in_mbs();
	int height_in_mb = m_slice->m_sps_active->Get_pic_height_in_mbs();

	bool left_edge_mb = (mb_idx % width_in_mb == 0);
	bool top_edge_mb = (mb_idx < width_in_mb);
	if (!top_edge_mb)
	{
		available_top = true;
		topIdx = block_idc_column == 0 ? (mb_idx - width_in_mb) : mb_idx;
	}
	else //ÉÏ±ßÑØºê¿é
	{
		if (block_idc_column == 0)
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
		leftIdx = block_idc_row == 0 ? (mb_idx - 1) : mb_idx;
	}
	else //×ó±ßÑØºê¿é
	{
		if (block_idc_row == 0)
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

int CMacroblock::get_top_neighbor_coeff_numbers(int topIdx, int block_idc_row, int block_idc_column)
{
	int nzCoeff = 0, target_idx_column = 0;

	if (topIdx == m_mb_idx)
	{
		target_idx_column = block_idc_column - 1;
		nzCoeff = m_residual->Get_sub_block_number_coeffs(block_idc_row, target_idx_column);
	}
	else
	{
		const CMacroblock *targetMB = m_slice->Get_macroblock_at_index(topIdx);
		if (targetMB->m_residual)
		{
			nzCoeff = targetMB->m_residual->Get_sub_block_number_coeffs(block_idc_row, 3);
		}
		else
		{
			nzCoeff = 0;
		}
	}

	return nzCoeff;
}

int CMacroblock::get_left_neighbor_coeff_numbers(int leftIdx, int block_idc_row, int block_idc_column)
{
	int nzCoeff = 0, target_idx_row = 0;
	if (leftIdx == m_mb_idx)
	{
		target_idx_row = block_idc_row - 1;
		nzCoeff = m_residual->Get_sub_block_number_coeffs(target_idx_row, block_idc_column);
	}
	else
	{
		const CMacroblock *targetMB = m_slice->Get_macroblock_at_index(leftIdx);
		if (targetMB->m_residual)
		{
			nzCoeff = targetMB->m_residual->Get_sub_block_number_coeffs(3, block_idc_column);
		}
		else
		{
			nzCoeff = 0;
		}
	}

	return nzCoeff;
}

int CMacroblock::get_top_neighbor_coeff_numbers_chroma(int topIdx, int component, int block_idc_row, int block_idc_column)
{
	int nzCoeff = 0, target_idx_column = 0;
	if (topIdx == m_mb_idx)
	{
		target_idx_column = block_idc_column - 1;
		nzCoeff = m_residual->Get_sub_block_number_coeffs_chroma(component, block_idc_row, target_idx_column);
	}
	else
	{
		const CMacroblock *targetMB = m_slice->Get_macroblock_at_index(topIdx);
		if (targetMB->m_residual)
		{
			nzCoeff = targetMB->m_residual->Get_sub_block_number_coeffs_chroma(component, block_idc_row, 1);
		} 
		else
		{
			nzCoeff = 0;
		}		
	}
	return nzCoeff;
}

int CMacroblock::get_left_neighbor_coeff_numbers_chroma(int leftIdx, int component, int block_idc_row, int block_idc_column)
{
	int nzCoeff = 0, target_idx_row = 0;
	if (leftIdx == m_mb_idx)
	{
		target_idx_row = block_idc_row - 1;
		nzCoeff = m_residual->Get_sub_block_number_coeffs_chroma(component, target_idx_row, block_idc_column);
	}
	else
	{
		const CMacroblock *targetMB = m_slice->Get_macroblock_at_index(leftIdx);
		if (targetMB->m_residual)
		{
			nzCoeff = targetMB->m_residual->Get_sub_block_number_coeffs_chroma(component, 1, block_idc_column);
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

int CMacroblock::get_intra_blocks_16x16()
{
	UINT8 up_left = 0, up[16] = { 0 }, left[16] = { 0 };
	NeighborBlocks neighbors = { 0 };

	get_neighbor_mb_availablility(neighbors);

	int err = get_reference_pixels_16(neighbors, up_left, up, left);
	if (err < 0)
	{
		return err;
	}

	err = get_pred_block_16(neighbors, up_left, up, left);
	if (err < 0)
	{
		return err;
	}

	err = reconstruct_block_16();
	if (err < 0)
	{
		return err;
	}

	dump_block16_info();

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_pred_block_16(NeighborBlocks &neighbors, UINT8 &up_left, UINT8 up[16], UINT8 left[16])
{
	UINT8 pred_blk16[16][16] = { { 0 } };
	bool available_left = neighbors.flags & 1, available_top = neighbors.flags & 2, available_top_left = neighbors.flags & 8;

	switch (m_intra16x16PredMode)
	{
	case VERT_PRED_16:
		if (!available_top)
		{
			return kPARSING_INVALID_PRED_MODE;
		}
		for (int column = 0; column < 16; column++)
		{
			for (int row = 0; row < 16; row++)
			{
				pred_blk16[column][row] = up[row];
			}
		}
		break;
	case HOR_PRED_16:
		if (!available_left)
		{
			return kPARSING_INVALID_PRED_MODE;
		}
		for (int column = 0; column < 16; column++)
		{
			for (int row = 0; row < 16; row++)
			{
				pred_blk16[column][row] = left[column];
			}
		}
		break;
	case DC_PRED_16:
	{
		UINT16 sum_up = 0, sum_left = 0, sum_dc = 0;
		for (int idx = 0; idx < 16; idx++)
		{
			if (available_left)
			{
				sum_left += left[idx];
			}
			if (available_top)
			{
				sum_up += up[idx];
			}
		}
		if (available_left && available_top)
		{
			sum_dc = (sum_up + sum_left + 16) >> 5;
		}
		else if (!available_top && available_left)
		{
			sum_dc = (sum_left + 8) >> 4;
		}
		else if (available_top && !available_left)
		{
			sum_dc = (sum_up + 8) >> 4;
		}
		else
		{
			sum_dc = 128;
		}

		for (int column = 0; column < 16; column++)
		{
			for (int row = 0; row < 16; row++)
			{
				pred_blk16[column][row] = sum_dc;
			}
		}

	}	break;
	case PLANE_16:
	{
		if (!available_left || !available_top || !available_top_left)
		{
			return kPARSING_INVALID_PRED_MODE;
		}
		int H = 0, V = 0, a = 0, b = 0, c = 0;
		for (int idx = 0; idx < 7; idx++)
		{
			H += (idx + 1) * (up[8 + idx] - up[6 - idx]);
			V += (idx + 1) * (left[8 + idx] - left[6 - idx]);
		}
		H += 8 * (up[15] - up_left);
		V += 8 * (left[15] - up_left);

		a = 16 * (up[15] + left[15]);
		b = (5 * H + 32) >> 6;
		c = (5 * V + 32) >> 6;

		for (int column = 0; column < 16; column++)
		{
			for (int row = 0; row < 16; row++)
			{
				pred_blk16[column][row] = max(0, min(((a + b*(row - 7) + c*(column - 7) + 16) >> 5), 255));
			}
		}
	}	break;
	default:
		return kPARSING_INVALID_PRED_MODE;
	}

	for (int idx = 0; idx < 16; idx++)
	{
		UINT8 row_idx = -1, col_idx = -1;
		block_index_to_position(idx, row_idx, col_idx);
		for (int blk_col = 0; blk_col < 4; blk_col++)
		{
			for (int blk_row = 0; blk_row < 4; blk_row++)
			{
				m_pred_block[idx][blk_col][blk_row] = pred_blk16[4 * col_idx + blk_col][4 * row_idx + blk_row];
			}
		}
	}

	return  kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_neighbor_mb_availablility(NeighborBlocks &neighbors)
{
	int mb_idx = m_mb_idx;
	int width_in_mb = m_slice->m_sps_active->Get_pic_width_in_mbs();
	int height_in_mb = m_slice->m_sps_active->Get_pic_height_in_mbs();

	bool left_edge_mb = (mb_idx % width_in_mb == 0);
	bool top_edge_mb = (mb_idx < width_in_mb);

	if (!left_edge_mb)
	{
		neighbors.flags |= 1;
		neighbors.left.target_mb_idx = mb_idx - 1;
	}
	else //×ó±ßÑØºê¿é
	{
		neighbors.flags &= 14;
	}

	if (!top_edge_mb)
	{
		neighbors.flags |= 2;
		neighbors.top.target_mb_idx = mb_idx - width_in_mb;
	}
	else //ÉÏ±ßÑØºê¿é
	{
		neighbors.flags &= 13;
	}

	if (!left_edge_mb && !top_edge_mb)
	{
		neighbors.flags |= 8;
		neighbors.top_left.target_mb_idx = mb_idx - width_in_mb - 1;
	} 
	else
	{
		neighbors.flags &= 7;
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_reference_pixels_16(const NeighborBlocks & neighbors, UINT8 &up_left, UINT8 up[16], UINT8 left[16])
{
	bool available_left = neighbors.flags & 1, available_top = neighbors.flags & 2, available_top_left = neighbors.flags & 8;
	CMacroblock *ref_mb = NULL;

	if (available_top_left)
	{
		ref_mb = m_slice->Get_macroblock_at_index(neighbors.top_left.target_mb_idx);
		up_left = ref_mb->m_reconstructed_block[3][3][3][3];
	} 
	else
	{
		up_left = 0;
	}

	if (available_top)
	{
		ref_mb = m_slice->Get_macroblock_at_index(neighbors.top.target_mb_idx);
		for (int idx = 0; idx < 4; idx++)
		{
			up[4 * idx + 0] = ref_mb->m_reconstructed_block[3][idx][3][0];
			up[4 * idx + 1] = ref_mb->m_reconstructed_block[3][idx][3][1];
			up[4 * idx + 2] = ref_mb->m_reconstructed_block[3][idx][3][2];
			up[4 * idx + 3] = ref_mb->m_reconstructed_block[3][idx][3][3];
		}
	} 
	else
	{
		memset(up, 0, 16);
	}

	if (available_left)
	{
		ref_mb = m_slice->Get_macroblock_at_index(neighbors.left.target_mb_idx);
		for (int idx = 0; idx < 4; idx++)
		{
			left[4 * idx + 0] = ref_mb->m_reconstructed_block[idx][3][0][3];
			left[4 * idx + 1] = ref_mb->m_reconstructed_block[idx][3][1][3];
			left[4 * idx + 2] = ref_mb->m_reconstructed_block[idx][3][2][3];
			left[4 * idx + 3] = ref_mb->m_reconstructed_block[idx][3][3][3];
		}
	} 
	else
	{
		memset(left, 0, 16);
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::reconstruct_block_16()
{
	for (UINT8 idx = 0; idx < 16; idx++)
	{
		reconstruct_block_of_idx(idx);
	}
	return kPARSING_ERROR_NO_ERROR;
}

void CMacroblock::dump_block16_info()
{
#if TRACE_CONFIG_MACROBLOCK
	g_tempFile << "Macroblock: " << to_string(m_mb_idx) << endl;

	UINT8 pred_block16[16][16] = { {0} }, recon_block[16][16] = { { 0 } };
	UINT8 blk_row = -1, blk_column = -1;
	for (int idx = 0; idx < 16; idx++)
	{
		block_index_to_position(idx, blk_row, blk_column);
		for (int column = 0; column < 4; column++)
		{
			for (int row = 0; row < 4; row++)
			{
				pred_block16[4 * blk_column + column][4 * blk_row + row] = m_pred_block[idx][column][row];
				recon_block[4 * blk_column + column][4 * blk_row + row] = m_reconstructed_block[blk_column][blk_row][column][row];
			}
		}

		m_residual->Dump_coeff_block(idx);
	}

#if TRACE_CONFIG_BLOCK_PRED_BLOCK
	g_tempFile << "Prediction Block: " << endl;
	for (int column = 0; column < 16; column++)
	{
		for (int row = 0; row < 16; row++)
		{
			g_tempFile << " " << to_string(pred_block16[column][row]);
		}
		g_tempFile << endl;
	}
#endif

#if TRACE_CONFIG_BLOCK_RECON_BLOCK
	g_tempFile << "Luma reconstructed block:" << endl;
	for (int column = 0; column < 16; column++)
	{
		for (int row = 0; row < 16; row++)
		{
			g_tempFile << " " << to_string(recon_block[column][row]);
		}
		g_tempFile << endl;
	}
#endif

#endif
}

int CMacroblock::get_intra_blocks_4x4()
{	
	int err = 0;
	for (UINT8 block_idx = 0; block_idx < 16; block_idx++)
	{
		err = get_pred_block_of_idx(block_idx);
		if (err < 0)
		{
			return err;
		}
		err = reconstruct_block_of_idx(block_idx);
		if (err < 0)
		{
			return err;
		}
		dump_block_info(block_idx);
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_pred_block_of_idx(UINT8 blkIdx)
{
	UINT8 topMBType = 0, leftMBType = 0;
	NeighborBlocks neighbors;
	UINT8 blk_row = 0, blk_column = 0;

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

	construct_pred_block(neighbors, blkIdx, this_intra_mode);

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::reconstruct_block_of_idx(UINT8 block_idx)
{
	int err = 0, temp = 0;
	UINT8 blk_row = -1, blk_column = -1;

	block_index_to_position(block_idx, blk_row, blk_column);

	for (int c = 0; c < 4; c++)
	{
		for (int r = 0; r < 4; r++)
		{
			temp = (m_pred_block[block_idx][c][r] << 6) + m_residual->m_residual_matrix_luma[block_idx][c][r];
			m_reconstructed_block[blk_column][blk_row][c][r] = max(0, min(255, (temp + 32) >> 6));
		}
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::construct_pred_block(NeighborBlocks neighbors, UINT8 blkIdx, int predMode)
{
	UINT8 refPixBuf[13] = { 0 }, predVal = 0, *refPtr = NULL;
	bool available_left = neighbors.flags & 1, available_top = neighbors.flags & 2, 
		available_top_right = neighbors.flags & 4, available_top_left = neighbors.flags & 8;
	int zVR = 0, zHD = 0, zHU = 0, refIndex = 0;

	get_reference_pixels(neighbors, blkIdx, refPixBuf);

	switch (predMode)
	{
	case VERT_PRED:
		for (int column = 0; column < 4; column++)
		{
			for (int row = 0; row < 4; row++)
			{
				m_pred_block[blkIdx][column][row] = refPixBuf[5 + row];
			}
		}
		break;
	case HOR_PRED:
		for (int column = 0; column < 4; column++)
		{
			for (int row = 0; row < 4; row++)
			{
				m_pred_block[blkIdx][column][row] = refPixBuf[3 - column];
			}
		}
		break;
	case DC_PRED:
		if (!available_top && !available_left)
		{
			predVal = 128;
		} 
		else if (available_top && available_left)
		{
			predVal = (refPixBuf[0] + refPixBuf[1] + refPixBuf[2] + refPixBuf[3] + refPixBuf[5] + refPixBuf[6] + refPixBuf[7] + refPixBuf[8] + 4) / 8;
		}
		else if (!available_top && available_left)
		{
			predVal = (refPixBuf[0] + refPixBuf[1] + refPixBuf[2] + refPixBuf[3] + 2) / 4;
		}
		else if (available_top && !available_left)
		{
			predVal = (refPixBuf[5] + refPixBuf[6] + refPixBuf[7] + refPixBuf[8] + 2) / 4;
		}

		for (int column = 0; column < 4; column++)
		{
			for (int row = 0; row < 4; row++)
			{
				m_pred_block[blkIdx][column][row] = predVal;
			}
		}
		break;
	case DIAG_DOWN_LEFT_PRED:
		refPtr = refPixBuf + 5;
		for (int column = 0; column < 4; column++)
		{
			for (int row = 0; row < 4; row++)
			{
				if (row == 3 && column == 3)
				{
					m_pred_block[blkIdx][column][row] = (refPtr[6] + 3 * refPtr[7] + 2) >> 2;
				} 
				else
				{
					m_pred_block[blkIdx][column][row] = (refPtr[row + column] + 2 * refPtr[row + column + 1] + refPtr[row + column + 2] + 2) >> 2;
				}
			}
		}
		break;
	case DIAG_DOWN_RIGHT_PRED:
		for (int column = 0; column < 4; column++)
		{
			for (int row = 0; row < 4; row++)
			{
				if (row == column)
				{
					m_pred_block[blkIdx][column][row] = (refPixBuf[5] + 2 * refPixBuf[4] + refPixBuf[3] + 2) >> 2;
				}
				else
				{
					refPtr = refPixBuf + 4;
					m_pred_block[blkIdx][column][row] = (refPtr[row - column - 1] + 2 * refPtr[row - column] + refPtr[row - column + 1] + 2) >> 2;
				}
			}
		}
		break;
	case VERT_RIGHT_PRED:
		refPtr = refPixBuf + 4;
		for (int column = 0; column < 4; column++)
		{
			for (int row = 0; row < 4; row++)
			{
				zVR = 2 * row - column;
				switch (zVR)
				{
				case 0:
				case 2:
				case 4:
				case 6:
					m_pred_block[blkIdx][column][row] = (refPtr[row - (column >> 1)] + refPtr[row - (column >> 1) + 1] + 1) >> 1;
					break;
				case 1:
				case 3:
				case 5:
					m_pred_block[blkIdx][column][row] = (refPtr[row - (column >> 1) - 1] + 2 * refPtr[row - (column >> 1)] + refPtr[row - (column >> 1) + 1] + 2) >> 2;
					break;
				case -1:
					m_pred_block[blkIdx][column][row] = (refPtr[-1] + 2 * refPtr[0] + refPtr[1] + 2) >> 2;
					break;
				case -2:
				case -3:
					m_pred_block[blkIdx][column][row] = (refPtr[-column] + 2 * refPtr[1 - column] + refPtr[2 - column] + 2) >> 2;
					break;
				default:
					break;
				}
			}
		}
		break;
	case HOR_DOWN_PRED:
		refPtr = refPixBuf + 4;
		for (int column = 0; column < 4; column++)
		{
			for (int row = 0; row < 4; row++)
			{
				zHD = 2 * column - row;
				refIndex = column - (row >> 1);
				switch (zHD)
				{
				case 0:
				case 2:
				case 4:
				case 6:
					m_pred_block[blkIdx][column][row] = (refPtr[-refIndex] + refPtr[-refIndex - 1] + 1) >> 1;
					break;
				case 1:
				case 3:
				case 5:
					m_pred_block[blkIdx][column][row] = (refPtr[-refIndex + 1] + 2 * refPtr[-refIndex] + refPtr[-refIndex - 1] + 2) >> 2;
					break;
				case -1:
					m_pred_block[blkIdx][column][row] = (refPtr[-1] + 2 * refPtr[0] + refPtr[1] + 2) >> 2;
					break;
				case -2:
				case -3:
					m_pred_block[blkIdx][column][row] = (refPtr[row] + 2 * refPtr[row - 1] + refPtr[row - 2] + 2) >> 2;
					break;
				default:
					break;
				}
			}
		}
		break;
	case VERT_LEFT_PRED:
		refPtr = refPixBuf + 5;
		for (int column = 0; column < 4; column++)
		{
			for (int row = 0; row < 4; row++)
			{
				if (column == 0 || column == 2)
				{
					m_pred_block[blkIdx][column][row] = (refPtr[row + (column >> 1)] + refPtr[row + (column >> 1) + 1] + 1) >> 1;
				} 
				else
				{
					m_pred_block[blkIdx][column][row] = (refPtr[row + (column >> 1)] + 2 * refPtr[row + (column >> 1) + 1] + refPtr[row + (column >> 1) + 2] + 2) >> 2;
				}
			}
		}
		break;
	case HOR_UP_PRED:
		refPtr = refPixBuf + 3;
		for (int column = 0; column < 4; column++)
		{
			for (int row = 0; row < 4; row++)
			{
				zHU = row + 2 * column;
				switch (zHU)
				{
				case 0:
				case 2:
				case 4:
					m_pred_block[blkIdx][column][row] = (refPtr[-(column + (row >> 1))] + refPtr[-(column + (row >> 1) + 1)] + 1) >> 1;
					break;
				case 1:
				case 3:
					m_pred_block[blkIdx][column][row] = (refPtr[-(column + (row >> 1))] + 2 * refPtr[-(column + (row >> 1) + 1)] + refPtr[-(column + (row >> 1) + 2)] + 2) >> 2;
					break;
				case 5:
					m_pred_block[blkIdx][column][row] = (refPixBuf[1] + 3 * refPixBuf[0] + 2) >> 2;
					break;
				default:
					m_pred_block[blkIdx][column][row] = refPixBuf[0];
					break;
				}
			}
		}
		break;
	default:
		break;
	}

#if TRACE_CONFIG_MACROBLOCK
	g_tempFile << "Macroblock: " << to_string(m_mb_idx) << " - Block: " << to_string(blkIdx) << endl;
	m_residual->Dump_coeff_block(blkIdx);

#if TRACE_CONFIG_BLOCK_REF_PIX
	g_tempFile << "Reference pixels:" << endl;
	for (int i = 0; i < 13; i++)
	{
		g_tempFile << " " << to_string(refPixBuf[i]);
	}
	g_tempFile << endl;
#endif

#endif

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_reference_pixels(NeighborBlocks neighbors, UINT8 blkIdx, UINT8 *refPixBuf)
{
	bool available_left = neighbors.flags & 1, available_top = neighbors.flags & 2, available_top_right = neighbors.flags & 4, available_top_left = neighbors.flags & 8;
	UINT8 blk_row = -1, blk_column = -1;
	CMacroblock *ref_mb = NULL;

	block_index_to_position(blkIdx, blk_row, blk_column);

	if (available_left)
	{
		ref_mb = m_slice->Get_macroblock_at_index(neighbors.left.target_mb_idx);
		refPixBuf[0] = ref_mb->m_reconstructed_block[neighbors.left.block_column][neighbors.left.block_row][3][3];
		refPixBuf[1] = ref_mb->m_reconstructed_block[neighbors.left.block_column][neighbors.left.block_row][2][3];
		refPixBuf[2] = ref_mb->m_reconstructed_block[neighbors.left.block_column][neighbors.left.block_row][1][3];
		refPixBuf[3] = ref_mb->m_reconstructed_block[neighbors.left.block_column][neighbors.left.block_row][0][3];
	} 
	else
	{
		refPixBuf[0] = refPixBuf[1] = refPixBuf[2] = refPixBuf[3] = 128;
	}

	if (available_top_left)
	{
		ref_mb = m_slice->Get_macroblock_at_index(neighbors.top_left.target_mb_idx);
		refPixBuf[4] = ref_mb->m_reconstructed_block[neighbors.top_left.block_column][neighbors.top_left.block_row][3][3];
	} 
	else
	{
		refPixBuf[4] = 128;
	}

	if (available_top)
	{
		ref_mb = m_slice->Get_macroblock_at_index(neighbors.top.target_mb_idx);
		refPixBuf[5] = ref_mb->m_reconstructed_block[neighbors.top.block_column][neighbors.top.block_row][3][0];
		refPixBuf[6] = ref_mb->m_reconstructed_block[neighbors.top.block_column][neighbors.top.block_row][3][1];
		refPixBuf[7] = ref_mb->m_reconstructed_block[neighbors.top.block_column][neighbors.top.block_row][3][2];
		refPixBuf[8] = ref_mb->m_reconstructed_block[neighbors.top.block_column][neighbors.top.block_row][3][3];
	} 
	else
	{
		refPixBuf[5] = refPixBuf[6] = refPixBuf[7] = refPixBuf[8] = 128;
	}

	if (available_top_right)
	{
		ref_mb = m_slice->Get_macroblock_at_index(neighbors.top_right.target_mb_idx);
		refPixBuf[9] = ref_mb->m_reconstructed_block[neighbors.top_right.block_column][neighbors.top_right.block_row][3][0];
		refPixBuf[10] = ref_mb->m_reconstructed_block[neighbors.top_right.block_column][neighbors.top_right.block_row][3][1];
		refPixBuf[11] = ref_mb->m_reconstructed_block[neighbors.top_right.block_column][neighbors.top_right.block_row][3][2];
		refPixBuf[12] = ref_mb->m_reconstructed_block[neighbors.top_right.block_column][neighbors.top_right.block_row][3][3];
	} 
	else
	{
		refPixBuf[9] = refPixBuf[10] = refPixBuf[11] = refPixBuf[12] = refPixBuf[8];
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_pred_mode_at_idx(UINT8 blkIdx)
{
	return m_intra_pred_mode[blkIdx];
}

void CMacroblock::dump_block_info(UINT8 blkIdx)
{
#if TRACE_CONFIG_MACROBLOCK

#if TRACE_CONFIG_BLOCK
	UINT8 blk_row = -1, blk_column = -1;
	block_index_to_position(blkIdx, blk_row, blk_column);

#if TRACE_CONFIG_BLOCK_PRED_BLOCK
	g_tempFile << "Prediction Block: " << endl;
	for (int column = 0; column < 4; column++)
	{
		for (int row = 0; row < 4; row++)
		{
			g_tempFile << " " << to_string(m_pred_block[blkIdx][column][row]);
		}
		g_tempFile << endl;
	}
#endif

#if TRACE_CONFIG_BLOCK_RECON_BLOCK
	g_tempFile << "Luma reconstructed block:" << endl;
	for (int column = 0; column < 4; column++)
	{
		for (int row = 0; row < 4; row++)
		{
			g_tempFile << " " << to_string(m_reconstructed_block[blk_column][blk_row][column][row]);
		}
		g_tempFile << endl;
	}
#endif

#endif

#endif

}

int CMacroblock::get_neighbor_blocks_availablility(NeighborBlocks &neighbors, int block_idc_row, int block_idc_column)
{
	int mb_idx = m_mb_idx;
	int width_in_mb = m_slice->m_sps_active->Get_pic_width_in_mbs();
	int height_in_mb = m_slice->m_sps_active->Get_pic_height_in_mbs();

	bool left_edge_mb = (mb_idx % width_in_mb == 0);
	bool top_edge_mb = (mb_idx < width_in_mb);
	bool right_edge_mb = ((mb_idx + 1) % width_in_mb == 0);

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

	if ((right_edge_mb && block_idc_row == 3) || (top_edge_mb && block_idc_column == 0) ||
		(block_idc_row == 3 && block_idc_column != 0) || (block_idc_row == 1 && block_idc_column == 1) || (block_idc_row == 1 && block_idc_column == 3))
	{
		neighbors.flags &= 11;
	}
	else
	{
		neighbors.flags |= 4;
		if (block_idc_row == 3 && block_idc_column == 0)
		{
			neighbors.top_right.target_mb_idx = mb_idx - width_in_mb + 1;
			neighbors.top_right.block_row = 0;
			neighbors.top_right.block_column = 3;
		}
		else if (block_idc_column == 0 && block_idc_row != 3)
		{
			neighbors.top_right.target_mb_idx = mb_idx - width_in_mb;
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

int CMacroblock::get_filtering_strength(int edge, int strength[16])
{
	int total_strength = 0;
	for (int idx = 0; idx < 16; idx++)
	{
		strength[idx] = (edge == 0) ? 4 : 3;
		total_strength += strength[idx];
	}

	return total_strength;
}

#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))

int ALPHA_TABLE[52] = { 0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,4,4,5,6,  7,8,9,10,12,13,15,17,  20,22,25,28,32,36,40,45,  50,56,63,71,80,90,101,113,  127,144,162,182,203,226,255,255 };
int BETA_TABLE[52] = { 0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,2,2,2,3,  3,3,3, 4, 4, 4, 6, 6,   7, 7, 8, 8, 9, 9,10,10,  11,11,12,12,13,13, 14, 14,   15, 15, 16, 16, 17, 17, 18, 18 };
int CLIP_TAB[52][5] =
{
	{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 1, 1 },{ 0, 0, 0, 1, 1 },{ 0, 0, 0, 1, 1 },{ 0, 0, 0, 1, 1 },{ 0, 0, 1, 1, 1 },{ 0, 0, 1, 1, 1 },{ 0, 1, 1, 1, 1 },
	{ 0, 1, 1, 1, 1 },{ 0, 1, 1, 1, 1 },{ 0, 1, 1, 1, 1 },{ 0, 1, 1, 2, 2 },{ 0, 1, 1, 2, 2 },{ 0, 1, 1, 2, 2 },{ 0, 1, 1, 2, 2 },{ 0, 1, 2, 3, 3 },
	{ 0, 1, 2, 3, 3 },{ 0, 2, 2, 3, 3 },{ 0, 2, 2, 4, 4 },{ 0, 2, 3, 4, 4 },{ 0, 2, 3, 4, 4 },{ 0, 3, 3, 5, 5 },{ 0, 3, 4, 6, 6 },{ 0, 3, 4, 6, 6 },
	{ 0, 4, 5, 7, 7 },{ 0, 4, 5, 8, 8 },{ 0, 4, 6, 9, 9 },{ 0, 5, 7,10,10 },{ 0, 6, 8,11,11 },{ 0, 6, 8,13,13 },{ 0, 7,10,14,14 },{ 0, 8,11,16,16 },
	{ 0, 9,12,18,18 },{ 0,10,13,20,20 },{ 0,11,15,23,23 },{ 0,13,17,25,25 }
};

int CMacroblock::filter_block_edge(int dir, int edge, int strength[16], int component)
{
	int filter_arr[8] = { 0 }, edge_pix_cnt = -1;
	int left_mb_idx = -1, top_mb_idx = -1, mb_idx = -1;
	int avg_qp = -1, index_a = -1, index_b = -1;
	int alpha_val = -1, beta_val = -1;
	int *clip_table = NULL;
	int filtered_pixel[8] = { 0 };
	NeighborBlocks neighbors = { 0 };

	get_neighbor_mb_availablility(neighbors);
	if (neighbors.flags & 1)
	{
		left_mb_idx = neighbors.left.target_mb_idx;
	}
	if (neighbors.flags & 2)
	{
		top_mb_idx = neighbors.top.target_mb_idx;
	}
	
	if (!dir) // Vertical
	{
		mb_idx = left_mb_idx < 0 ? m_mb_idx : left_mb_idx;
	} 
	else //Horizontal
	{
		mb_idx = top_mb_idx < 0 ? m_mb_idx : top_mb_idx;
	}

	if (component) // Chroma
	{
		edge_pix_cnt = 8;
	}
	else // Luma
	{
		edge_pix_cnt = 16;
		avg_qp = (m_mb_qp + m_slice->Get_macroblock_at_index(mb_idx)->m_mb_qp + 1) >> 1;

		index_a = IClip(0, MAX_QP, avg_qp + m_mb_alpha_c0_offset);
		index_b = IClip(0, MAX_QP, avg_qp + m_mb_beta_offset);

		alpha_val = ALPHA_TABLE[index_a];
		beta_val = BETA_TABLE[index_b];
		clip_table = CLIP_TAB[index_a];

		for (int idx = 0; idx < edge_pix_cnt; idx++)
		{
			get_edge_pixel_item(dir, mb_idx, edge, idx, 1, filter_arr);
			filter_pixel(filtered_pixel, alpha_val, beta_val, clip_table, strength, filter_arr, idx, component);
			set_edge_pixel_item(filtered_pixel, dir, mb_idx, edge, idx, 1, filter_arr);
		}
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_edge_pixel_item(int dir, int target_mb_idx, int edge, int pix_idx, int luma, int pixel_arr[8])
{
	CMacroblock *target = NULL;	
	int neighbor_blk_idx = -1, blk_idx = pix_idx / 4, pix_pos = pix_idx % 4;
	/*
	For luma:
		pix_idx ranges [0,15], so blk_idx means the block index the pixel belongs to, and pix_pos means the pixel index in the sub block.
	*/
	if (edge)// Internal Filtering
	{
		target = this;
		neighbor_blk_idx = edge - 1;
	}
	else
	{
		target = m_slice->Get_macroblock_at_index(target_mb_idx);
		neighbor_blk_idx = 3;
	}

	if (luma)
	{
		if (!dir) // Vertical
		{
			for (int idx = 0; idx < 4; idx++)
			{
				pixel_arr[idx] = target->m_reconstructed_block[blk_idx][neighbor_blk_idx][pix_pos][idx];
			}
			for (int idx = 4; idx < 8; idx++)
			{
				pixel_arr[idx] = m_reconstructed_block[blk_idx][edge][pix_pos][idx-4];
			}
		}
		else //Horizontal
		{
			for (int idx = 0; idx < 4; idx++)
			{
				pixel_arr[idx] = target->m_reconstructed_block[neighbor_blk_idx][blk_idx][idx][pix_pos];
			}
			for (int idx = 4; idx < 8; idx++)
			{
				pixel_arr[idx] = m_reconstructed_block[edge][blk_idx][idx - 4][pix_pos];
			}
		}		
	}


	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::filter_pixel(int *pix_vals, int alpha_val, int beta_val, int *clip_table, int strength[16], int pixel_arr[8], int strength_idx, int component)
{
	int delta = -1, abs_delta = -1;
	int C0 = -1, c0 = -1, RL0 = -1, dif = -1;
	int filter_strength = strength[strength_idx];

	for (int i = 0; i < 8; i++)
	{
		pix_vals[i] = pixel_arr[i];
	}

	if (!filter_strength)
	{
		return kPARSING_ERROR_NO_ERROR;
	}

	RL0 = pixel_arr[4] + pixel_arr[3];
	delta = pixel_arr[4] - pixel_arr[3];
	abs_delta = abs(delta);
	if (abs_delta >= alpha_val)
	{
		return kPARSING_ERROR_NO_ERROR;
	}

	C0 = clip_table[filter_strength];
	int right_diff = abs(pixel_arr[4] - pixel_arr[5]) - beta_val, left_diff = abs(pixel_arr[3] - pixel_arr[2]) - beta_val;
	int right_diff2_negative = 0, left_diff2_negative = 0, small_gleft_diff2_negative = 0;
	if ((right_diff & left_diff) >= 0)
	{
		return kPARSING_ERROR_NO_ERROR;
	}

	if (!component) // Luma
	{
		right_diff2_negative = (abs(pixel_arr[4] - pixel_arr[6]) - beta_val) < 0;
		left_diff2_negative = (abs(pixel_arr[3] - pixel_arr[1]) - beta_val) < 0;

		if (filter_strength == 3)
		{
			c0 = C0 + right_diff2_negative + left_diff2_negative;
			dif = IClip(-c0, c0, ((delta << 2) + (pixel_arr[2] - pixel_arr[5]) + 4) >> 3);
			pix_vals[3] = IClip(0, 255, pixel_arr[3] + dif);
			pix_vals[4] = IClip(0, 255, pixel_arr[4] - dif);

			if (left_diff2_negative)
			{
				pix_vals[2] += IClip(-C0, C0, (pixel_arr[1] + ((RL0 + 1) >> 1) - (pixel_arr[2] << 1)) >> 1);
			}
			if (right_diff2_negative)
			{
				pix_vals[5] += IClip(-C0, C0, (pixel_arr[6] + ((RL0 + 1) >> 1) - (pixel_arr[5] << 1)) >> 1);
			}
		}
		else if (filter_strength == 4)
		{
			small_gleft_diff2_negative = ( abs_delta < ((alpha_val >> 2) + 2) );
			right_diff2_negative &= small_gleft_diff2_negative;
			left_diff2_negative &= small_gleft_diff2_negative;

			pix_vals[4] = right_diff2_negative ? (pix_vals[2] + ((pix_vals[5] + RL0) << 1) + pix_vals[6] + 4) >> 3 : ((pix_vals[5] << 1) + pix_vals[4] + pix_vals[2] + 2) >> 2;
			pix_vals[3] = left_diff2_negative  ? (pix_vals[5] + ((pix_vals[2] + RL0) << 1) + pix_vals[1] + 4) >> 3 : ((pix_vals[2] << 1) + pix_vals[3] + pix_vals[5] + 2) >> 2;

			pix_vals[5] = right_diff2_negative ? (pix_vals[6] + pix_vals[5] + pix_vals[4] + pix_vals[3] + 2) >> 2 : pix_vals[5];
			pix_vals[2] = left_diff2_negative  ? (pix_vals[1] + pix_vals[2] + pix_vals[3] + pix_vals[4] + 2) >> 2 : pix_vals[2];

			pix_vals[6] = right_diff2_negative ? (((pix_vals[7] + pix_vals[6]) << 1) + pix_vals[6] + pix_vals[5] + RL0 + 4) >> 3 : pix_vals[6];
			pix_vals[1] = left_diff2_negative  ? (((pix_vals[0] + pix_vals[1]) << 1) + pix_vals[1] + pix_vals[2] + RL0 + 4) >> 3 : pix_vals[1];
		}
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::set_edge_pixel_item(int *pix_vals, int dir, int target_mb_idx, int edge, int pix_idx, int luma, int pixel_arr[8])
{
	CMacroblock *target = NULL;
	int neighbor_blk_idx = -1, blk_idx = pix_idx / 4, pix_pos = pix_idx % 4;

	if (edge)// Internal Filtering
	{
		target = this;
		neighbor_blk_idx = edge - 1;
	}
	else
	{
		target = m_slice->Get_macroblock_at_index(target_mb_idx);
		neighbor_blk_idx = 3;
	}

	if (luma)
	{
		if (!dir) // Vertical
		{
			for (int idx = 0; idx < 4; idx++)
			{
				target->m_reconstructed_block[blk_idx][neighbor_blk_idx][pix_pos][idx] = pix_vals[idx];
			}
			for (int idx = 4; idx < 8; idx++)
			{
				m_reconstructed_block[blk_idx][edge][pix_pos][idx - 4] = pix_vals[idx];
			}
		}
		else //Horizontal
		{
			for (int idx = 0; idx < 4; idx++)
			{
				target->m_reconstructed_block[neighbor_blk_idx][blk_idx][idx][pix_pos] = pix_vals[idx];
			}
			for (int idx = 4; idx < 8; idx++)
			{
				m_reconstructed_block[edge][blk_idx][idx - 4][pix_pos] = pix_vals[idx];
			}
		}
	}

	return kPARSING_ERROR_NO_ERROR;
}
