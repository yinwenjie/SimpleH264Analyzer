#include "stdafx.h"
#include "Macroblock.h"
#include "SeqParamSet.h"
#include "PicParamSet.h"
#include "SliceHeader.h"
#include "SliceStruct.h"

#include "CAVLC_Defines.h"
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

	m_pps_active = NULL;
	m_slice = NULL;

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
	}

	interpret_mb_mode();

	// parse coefficients...
	m_coeffArray = new MacroBlockCoeffArray;
	get_luma_coeffs();

	m_mbDataSize = m_bypeOffset * 8 + m_bitOffset - m_mbDataSize;
	return m_mbDataSize;
}

void CMacroblock::Dump_macroblock_info()
{
#if TRACE_CONFIG_LOGOUT

#if TRACE_CONFIG_MACROBLOCK

	g_traceFile << "***** MB: " << to_string(m_mb_idx) << " *****" <<endl;
	g_traceFile << "mb_type: " << to_string(m_mb_type) << endl;
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

	g_traceFile << "intra_chroma_pred_mode: " << to_string(m_intra_chroma_pred_mode) << endl;
	if (m_mb_type == 0 || m_mb_type == 25)
	{
		g_traceFile << "coded_block_pattern: " << to_string(m_coded_block_pattern) << endl;
	}
	if (m_cbp_luma > 0 || m_cbp_chroma > 0 || (m_mb_type > 0 && m_mb_type < 25))
	{
		g_traceFile << "mb_qp_delta: " << to_string(m_mb_qp_delta) << endl;
	}
	g_traceFile << "***************" << endl;
#endif

#endif
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

int CMacroblock::get_luma_coeffs()
{
	int b8 = 0;
	for (int block_y = 0; block_y < 4; block_y += 2)
	{
		for (int block_x = 0; block_x < 4; block_x += 2)
		{
			// 16x16 -> 4 * 8x8
			
			// CAVLC
			if (m_pps_active->Get_entropy_coding_flag() == 0)
			{
				for (int block_sub_idc_y = block_y; block_sub_idc_y < block_y + 2; block_sub_idc_y++)
				{
					for (int block_sub_idc_x = block_x; block_sub_idc_x < block_x + 2; block_sub_idc_x++)
					{
						// 8x8 -> 4 * 4x4
						b8 = 2 * (block_y / 2) + block_x / 2;
						if ( m_coded_block_pattern & (1 << b8))
						{
							get_luma4x4_coeffs(block_sub_idc_x, block_sub_idc_y);
						} 
						else
						{
							// empty block...
						}
					}
				}
			} 
			// CABAC
			else
			{
				
			}
		}
	}

	return 0;
}

int CMacroblock::get_luma4x4_coeffs(int block_idc_x, int block_idc_y)
{
	int block_type = (m_mb_type == I16MB || m_mb_type == IPCM) ? LUMA_INTRA16x16AC : LUMA;
	int max_coeff_num = 0, numCoeff_vlcIdx = 0;

	int numCoeff = 0, trailingOnes = 0;

	switch (block_type)
	{
	case LUMA:
		max_coeff_num = 16;
		break;
	case LUMA_INTRA16x16DC:
		max_coeff_num = 15;
		break;
	case LUMA_INTRA16x16AC:
		max_coeff_num = 15;
		break;
	default:
		break;
	}

	int numberCurrent = get_number_current(block_idc_x, block_idc_y, 1);
	if (numberCurrent < 2)
	{
		numCoeff_vlcIdx = 0;
	}

	get_numCoeff_and_trailingOnes(numCoeff, trailingOnes, numCoeff_vlcIdx);
	
	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_numCoeff_and_trailingOnes(int &totalCoeff, int &trailingOnes, int numCoeff_vlcIdx)
{
	int err = 0;
	int *lengthTable = NULL, *codeTable = NULL;

	if (numCoeff_vlcIdx < 3)
	{
		lengthTable = &coeffTokenTable_Length[numCoeff_vlcIdx][0][0];
		codeTable = &coeffTokenTable_Code[numCoeff_vlcIdx][0][0];
		err = search_for_value_in_2D_table(totalCoeff, trailingOnes, lengthTable, codeTable, 17, 4);
		if (err < 0)
		{
			return err;
		}
	} 
	else
	{
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CMacroblock::get_number_current(int block_idc_x, int block_idc_y, int luma)
{
	int nC = 0;
	bool available_top = false, available_left = false;

	get_neighbor_available(available_top, available_left, block_idc_x, block_idc_y, luma);
	
	if (!available_left && !available_top)
	{
		nC = 0;
	}
	// Todo: other condition

	return nC;
}

void CMacroblock::get_neighbor_available(bool &available_top, bool &available_left, int block_idc_x, int block_idc_y, int luma)
{
	int mb_idx = m_mb_idx;
	int maxWH = (luma ? 16 : 8);
	int width_in_mb = m_slice->m_sps_active->Get_pic_width_in_mbs();
	int height_in_mb = m_slice->m_sps_active->Get_pic_height_in_mbs();

	bool left_edge_mb = (mb_idx % width_in_mb == 0);
	bool top_edge_mb = (mb_idx < width_in_mb);
	if (!top_edge_mb)
	{
		available_top = true;
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
		}
	}

	if (!left_edge_mb)
	{
		available_left = true;
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
		}
	}
	
	return;
}

int CMacroblock::search_for_value_in_2D_table(int &value1, int &value2, int *lengthTable, int *codeTable, int tableWidth, int tableHeight)
{
	int err = 0;
	int codeLen = 0, code = 0;
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
