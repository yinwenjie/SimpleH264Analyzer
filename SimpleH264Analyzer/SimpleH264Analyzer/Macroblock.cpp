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
