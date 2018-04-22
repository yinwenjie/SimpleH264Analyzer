#include "stdafx.h"
#include "Residual.h"
#include "Macroblock.h"
#include "Macroblock_Defines.h"
#include "PicParamSet.h"
#include "CAVLC_Defines.h"

using namespace std;

void swap(int &x, int &y)
{
	int temp = x;
	x = y;
	y = temp;
}

CResidual::CResidual(UINT8 *pSODB, UINT32 offset, CMacroblock *mb)
{
	m_macroblock_belongs = mb;
	m_pSODB = pSODB;
	m_bypeOffset = offset / 8;
	m_bitOffset = offset % 8;

	for (int idx = 0; idx < 16; idx++)
	{
		for (int i = 0; i < 16; i++)
		{
			for (int j = 0; j < 16; j++)
			{
				m_coeff_matrix_luma[idx][i][j] = 0;
				m_residual_matrix_luma[idx][i][j] = 0;
			}
		}
	}

	for (int chrIdx = 0; chrIdx < 2; chrIdx++)
	{
		for (int idx = 0; idx < 4; idx++)
		{
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					m_coeff_matrix_chroma[chrIdx][idx][i][j] = 0;
					m_residual_matrix_chroma[chrIdx][idx][i][j] = 0;
				}
			}
		}
	}
}

CResidual::~CResidual()
{
}

int CResidual::Parse_macroblock_residual(UINT32 &dataLength)
{
	UINT8 cbp_luma = m_macroblock_belongs->m_cbp_luma;
	UINT8 cbp_chroma = m_macroblock_belongs->m_cbp_chroma;
	UINT32 originOffset = 8 * m_bypeOffset + m_bitOffset;

	m_qp = m_macroblock_belongs->m_mb_qp;

	if (m_macroblock_belongs->m_mb_type == I16MB)
	{
		parse_luma_residual_16x16_DC();
		Dump_residual_luma16x16_DC();
		if (cbp_luma)
		{
			parse_luma_residual(LUMA_INTRA16x16AC, cbp_luma);
			Dump_residual_luma(LUMA_INTRA16x16AC);
		}
	} 
	else if (m_macroblock_belongs->m_mb_type == I4MB)
	{
		if (cbp_luma)
		{
			parse_luma_residual(LUMA, cbp_luma);		
			Dump_residual_luma(LUMA);
		}
	}	

	if (cbp_chroma)
	{
		parse_chroma_residual(cbp_chroma);
		Dump_residual_chroma(cbp_chroma);
	}

	dataLength = 8 * m_bypeOffset + m_bitOffset - originOffset;
	return kPARSING_ERROR_NO_ERROR;
}

UINT8 CResidual::Get_sub_block_number_coeffs(int block_idc_row, int block_idc_column)
{	
	UINT8 numCoeff = 0;

	if (m_macroblock_belongs->m_mb_type == I4MB)
	{
		numCoeff = luma_residual[block_idc_column][block_idc_row].numCoeff;
	}
	else if (m_macroblock_belongs->m_mb_type = I16MB)
	{
		if (block_idc_row == 0 && block_idc_column == 0)
		{
			numCoeff = luma_residual16x16_DC.numCoeff;
		}

		if (m_macroblock_belongs->m_cbp_luma)
		{
			numCoeff = luma_residual16x16_AC[block_idc_column][block_idc_row].numCoeff;
		}
	}

	return numCoeff;
}

UINT8 CResidual::Get_sub_block_number_coeffs_chroma(int component, int block_idc_row, int block_idc_column)
{
	return chroma_AC_residual[component][block_idc_column][block_idc_row].numCoeff;
}

void CResidual::Restore_coeff_matrix()
{
	UINT8 cbp_luma = m_macroblock_belongs->m_cbp_luma;
	UINT8 cbp_chroma = m_macroblock_belongs->m_cbp_chroma;

	if (m_macroblock_belongs->m_mb_type == I4MB)
	{
		for (int blk8Idx = 0; blk8Idx < 4; blk8Idx++)
		{
			if (cbp_luma & (1 << blk8Idx))
			{
				restore_8x8_coeff_block_luma(m_coeff_matrix_luma, blk8Idx, LUMA);
			}
		}
	}
	else if (m_macroblock_belongs->m_mb_type == I16MB)
	{
		restore_16x16_coeff_block_luma_DC(m_coeff_matrix_luma);
		for (int blk8Idx = 0; blk8Idx < 4; blk8Idx++)
		{
			if (cbp_luma & (1 << blk8Idx))
			{
				restore_8x8_coeff_block_luma(m_coeff_matrix_luma, blk8Idx, LUMA_INTRA16x16AC);
			}
		}
	}

	for (int idx = 0; idx < 16; idx++)
	{
		coeff_invers_transform(m_coeff_matrix_luma[idx], m_residual_matrix_luma[idx]);
	}
	
	if (cbp_chroma > 0)
	{
		for (int blk8Idx = 0; blk8Idx < 2; blk8Idx++)
		{
			restore_8x8_coeff_block_chroma_DC(m_coeff_matrix_chroma, blk8Idx);
		}

		if (cbp_chroma > 1)
		{
			for (int chrIdx = 0; chrIdx < 2; chrIdx++)
			{
				restore_8x8_coeff_block_chroma_AC(m_coeff_matrix_chroma, chrIdx);
			}
		}
	}
}

void CResidual::Dump_coeff_block(int blk_idx)
{
#if TRACE_CONFIG_BLOCK_COEF_BLK_LUMA
	g_tempFile << "Coefficients:" << endl;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			g_tempFile << " " << to_string(m_coeff_matrix_luma[blk_idx][i][j]);
		}
		g_tempFile << endl;
	}
#endif
}

void CResidual::Dump_residual_luma(int blockType)
{
#if TRACE_CONFIG_LOGOUT

#if TRACE_CONFIG_MACROBLOCK_RESIDUAL

	Coeff4x4Block (*targetBlock)[4] = NULL;
	string name;

	if (blockType == LUMA)
	{
		g_traceFile << "Luma Residual 4x4:" << endl;
		targetBlock = luma_residual;
		name = "Luma";
	} 
	else if (blockType == LUMA_INTRA16x16AC)
	{
		g_traceFile << "Luma Residual 16x16 AC:" << endl;
		targetBlock = luma_residual16x16_AC;
		name = "Luma16x16AC";
	}

	
	for (int outRow = 0; outRow < 4; outRow += 2)
	{
		for (int outColumn = 0; outColumn < 4; outColumn += 2)
		{
			for (int row = outRow; row < outRow + 2; row++)
			{
				for (int column = outColumn; column < outColumn + 2; column++)
				{
					g_traceFile << name <<"[" << row << "][" << column << "]: ";
					if (targetBlock[row][column].emptyBlock)
					{
						g_traceFile << "Empty." << endl;
					}
					else
					{
						g_traceFile << "numCoeff: " << to_string(targetBlock[row][column].numCoeff) << "\ttrailingOnes: " << to_string(targetBlock[row][column].trailingOnes) << endl;
						if (targetBlock[row][column].numCoeff)
						{
							if (targetBlock[row][column].trailingOnes)
							{
								g_traceFile << "\ttrailingSign: ";
								for (int idx = 0; idx < targetBlock[row][column].trailingOnes; idx++)
								{
									g_traceFile << to_string(targetBlock[row][column].trailingSign[idx]) << " ";
								}
								g_traceFile << endl;
							}

							int levelCnt = targetBlock[row][column].numCoeff - targetBlock[row][column].trailingOnes;
							if (levelCnt)
							{
								g_traceFile << "\tlevels: ";
								for (int idx = 0; idx < levelCnt; idx++)
								{
									g_traceFile << to_string(targetBlock[row][column].levels[idx]) << " ";
								}
								g_traceFile << endl;
							}
						}
					}
				}
			}			
		}
	}

#endif

#endif
}

void CResidual::Dump_residual_chroma(UINT8 cbp_chroma)
{
	Dump_residual_chroma_DC();

	if (cbp_chroma & 2)
	{
		Dump_residual_chroma_AC();
	}
}

void CResidual::Dump_residual_chroma_DC()
{
#if TRACE_CONFIG_LOGOUT

#if TRACE_CONFIG_MACROBLOCK_RESIDUAL

	g_traceFile << "Chroma Residual DC:" << endl;
	for (int chromaIdx = 0; chromaIdx < 2; chromaIdx++)
	{
		g_traceFile << "ChromaDC[" << chromaIdx << "]: ";
		if (chroma_DC_residual[chromaIdx].emptyBlock)
		{
			g_traceFile << "Empty." << endl;
		} 
		else
		{
			g_traceFile << "numCoeff: " << to_string(chroma_DC_residual[chromaIdx].numCoeff) << "\ttrailingOnes: " << to_string(chroma_DC_residual[chromaIdx].trailingOnes) << endl;
			if (chroma_DC_residual[chromaIdx].numCoeff != 0)
			{
				if (chroma_DC_residual[chromaIdx].trailingOnes != 0)
				{
					g_traceFile << "\ttrailingSign: ";
					for (int idx = 0; idx < chroma_DC_residual[chromaIdx].trailingOnes; idx++)
					{
						g_traceFile << to_string(chroma_DC_residual[chromaIdx].trailingSign[idx]) << " ";
					}
					g_traceFile << endl;
				}

				int levelCnt = chroma_DC_residual[chromaIdx].numCoeff - chroma_DC_residual[chromaIdx].trailingOnes;
				if (levelCnt)
				{
					g_traceFile << "\tlevels: ";
					for (int idx = 0; idx < levelCnt; idx++)
					{
						g_traceFile << to_string(chroma_DC_residual[chromaIdx].levels[idx]) << " ";
					}
					g_traceFile << endl;
				}
			}
		}
	}
#endif

#endif
}

void CResidual::Dump_residual_chroma_AC()
{
#if TRACE_CONFIG_LOGOUT

#if TRACE_CONFIG_MACROBLOCK_RESIDUAL

	g_traceFile << "Chroma Residual AC:" << endl;
	for (int chromaIdx = 0; chromaIdx < 2; chromaIdx++)
	{
		for (int block_idx_y = 0; block_idx_y < 2; block_idx_y++)
		{
			for (int block_idx_x = 0; block_idx_x < 2; block_idx_x++)
			{
				g_traceFile << "ChromaAC[" << chromaIdx << "][" << to_string(block_idx_y) << "][" << to_string(block_idx_x)<< "]: ";
				if (chroma_AC_residual[chromaIdx][block_idx_y][block_idx_x].emptyBlock)
				{
					g_traceFile << "Empty." << endl;
				} 
				else
				{
					g_traceFile << "numCoeff: " << to_string(chroma_AC_residual[chromaIdx][block_idx_y][block_idx_x].numCoeff) << "\ttrailingOnes: " << to_string(chroma_AC_residual[chromaIdx][block_idx_y][block_idx_x].trailingOnes) << endl;
					if (chroma_AC_residual[chromaIdx][block_idx_y][block_idx_x].numCoeff != 0)
					{
						if (chroma_AC_residual[chromaIdx][block_idx_y][block_idx_x].trailingOnes != 0)
						{
							g_traceFile << "\ttrailingSign: ";
							for (int idx = 0; idx < chroma_AC_residual[chromaIdx][block_idx_y][block_idx_x].trailingOnes; idx++)
							{
								g_traceFile << to_string(chroma_AC_residual[chromaIdx][block_idx_y][block_idx_x].trailingSign[idx]) << " ";
							}
							g_traceFile << endl;
						}

						int levelCnt = chroma_AC_residual[chromaIdx][block_idx_y][block_idx_x].numCoeff - chroma_AC_residual[chromaIdx][block_idx_y][block_idx_x].trailingOnes;
						if (levelCnt)
						{
							g_traceFile << "\tlevels: ";
							for (int idx = 0; idx < levelCnt; idx++)
							{
								g_traceFile << to_string(chroma_AC_residual[chromaIdx][block_idx_y][block_idx_x].levels[idx]) << " ";
							}
							g_traceFile << endl;
						}
					}
				}
			}
		}
	}
#endif

#endif
}

void CResidual::Dump_residual_luma16x16_DC()
{
#if TRACE_CONFIG_LOGOUT

#if TRACE_CONFIG_MACROBLOCK_RESIDUAL
	g_traceFile << "Luma Residual 16x16 DC:" << endl;
	g_traceFile << "Luma16x16DC: ";
	if (luma_residual16x16_DC.emptyBlock == true)
	{
		g_traceFile << "Empty." << endl;
	} 
	else
	{
		g_traceFile << "numCoeff: " << to_string(luma_residual16x16_DC.numCoeff) << "\ttrailingOnes: " << to_string(luma_residual16x16_DC.trailingOnes) << endl;
		if (luma_residual16x16_DC.numCoeff != 0)
		{
			if (luma_residual16x16_DC.trailingOnes != 0)
			{
				g_traceFile << "\ttrailingSign: ";
				for (int idx = 0; idx < luma_residual16x16_DC.trailingOnes; idx++)
				{
					g_traceFile << to_string(luma_residual16x16_DC.trailingSign[idx]) << " ";
				}
				g_traceFile << endl;
			}
			int levelCnt = luma_residual16x16_DC.numCoeff - luma_residual16x16_DC.trailingOnes;
			if (levelCnt)
			{
				g_traceFile << "\tlevels: ";
				for (int idx = 0; idx < levelCnt; idx++)
				{
					g_traceFile << to_string(luma_residual16x16_DC.levels[idx]) << " ";
				}
				g_traceFile << endl;
			}
		}
	}

#endif

#endif
}

int CResidual::parse_luma_residual(int blockType, UINT8 cbp_luma)
{
	int err = 0;
	int idx8x8 = 0, block_row = 0, block_column = 0, block_sub_row_idc = 0, block_sub_column_idc = 0;

	for (block_column = 0; block_column < 4; block_column += 2)
	{
		for (block_row = 0; block_row < 4; block_row += 2)
		{
			// 16x16 -> 4 * 8x8						
			if (m_macroblock_belongs->Get_pps_active()->Get_entropy_coding_flag() == false)
			{
				// CAVLC
				for (block_sub_column_idc = block_column; block_sub_column_idc < block_column + 2; block_sub_column_idc++)
				{
					for (block_sub_row_idc = block_row; block_sub_row_idc < block_row + 2; block_sub_row_idc++)
					{
						// 8x8 -> 4 * 4x4
						idx8x8 = 2 * (block_column / 2) + block_row / 2;
						if (cbp_luma & (1 << idx8x8))
						{
							luma_residual[block_sub_column_idc][block_sub_row_idc].emptyBlock = false;
							err = get_luma4x4_coeffs(blockType, block_sub_row_idc, block_sub_column_idc);
							if (err < 0)
							{
								return err;
							}
						} 
						else
						{
							luma_residual[block_sub_column_idc][block_sub_row_idc].emptyBlock = true;
						}
					}
				}
			}
			else
			{
				// CABAC
			}
		}
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CResidual::get_luma4x4_coeffs(int blockType, int block_idc_row, int block_idc_column)
{
	int err = 0;
	int mb_type = m_macroblock_belongs->m_mb_type;
	int max_coeff_num = 0;
	int numCoeff_vlcIdx = 0, prefixLength = 0, suffixLength = 0, level_prefix = 0, level_suffix = 0;
	int levelSuffixSize = 0, levelCode = 0, i = 0;

	Coeff4x4Block *targetBlock = NULL;
	
	switch (blockType)
	{
	case LUMA:
		max_coeff_num = 16;
		targetBlock = &luma_residual[block_idc_column][block_idc_row];
		break;
	case LUMA_INTRA16x16DC:
		max_coeff_num = 16;
		targetBlock = &luma_residual16x16_DC;
		break;
	case LUMA_INTRA16x16AC:
		max_coeff_num = 15;
		targetBlock = &luma_residual16x16_AC[block_idc_column][block_idc_row];
		break;
	default:
		break;
	}

	int numberCurrent = m_macroblock_belongs->Get_number_current(block_idc_row, block_idc_column);
	if (numberCurrent < 2)
	{
		numCoeff_vlcIdx = 0;
	}
	else if (numberCurrent < 4)
	{
		numCoeff_vlcIdx = 1;
	}
	else if (numberCurrent < 8)
	{
		numCoeff_vlcIdx = 2;
	}
	else
	{
		numCoeff_vlcIdx = 3;
	}

	// NumCoeff & TrailingOnes...
	UINT8 numCoeff = 0, trailingOnes = 0;
	int token = 0;
	err = get_numCoeff_and_trailingOnes(numCoeff, trailingOnes, token, numCoeff_vlcIdx);
	if (err < 0)
	{
		return err;
	}
	else
	{
		targetBlock->coeffToken = token;
		targetBlock->numCoeff = numCoeff;
		targetBlock->trailingOnes = trailingOnes;
	}

	if (numCoeff) //包含非0系数
	{
		if (trailingOnes) //拖尾系数
		{
			//读取拖尾系数符号
			int signValue = Get_uint_code_num(m_pSODB, m_bypeOffset, m_bitOffset, trailingOnes);
			int trailingCnt = trailingOnes;
			for (int coeffIdx = 0; coeffIdx < trailingOnes; coeffIdx++)
			{
				trailingCnt--;
				if ((signValue >> trailingCnt) & 1)
				{
					targetBlock->trailingSign[coeffIdx] = -1;
				}
				else
				{
					targetBlock->trailingSign[coeffIdx] = 1;
				}
			}
		}

		//读取解析level值
		int level = 0;
		if (numCoeff > 10 && trailingOnes < 3)
		{
			//根据上下文初始化suffixLength
			suffixLength = 1;
		}
		for (int k = 0; k <= numCoeff - 1 - trailingOnes; k++)
		{
			err = get_coeff_level(level, k, trailingOnes, suffixLength);
			if (err < 0)
			{
				return err;
			}

			if (suffixLength == 0)
			{
				suffixLength = 1;
			}

			if ((abs(level) >(3 << (suffixLength - 1))) && (suffixLength < 6))
			{
				suffixLength++;
			}

			targetBlock->levels[k] = level;
		}

		// 读取解析run
		UINT8 zerosLeft = 0, totalZeros = 0, run = 0;
		if (numCoeff < max_coeff_num)
		{
			err = get_total_zeros(totalZeros, numCoeff - 1);
			if (err < 0)
			{
				return err;
			}
		}
		else
		{
			totalZeros = 0;
		}
		targetBlock->totalZeros = totalZeros;

		//读取解析run_before
		int runBefore_vlcIdx = 0;
		i = numCoeff - 1;
		zerosLeft = totalZeros;
		if (zerosLeft > 0 && i > 0)
		{
			do
			{
				runBefore_vlcIdx = (zerosLeft - 1 < 6 ? zerosLeft - 1 : 6);
				err = get_run_before(run, runBefore_vlcIdx);
				if (err < 0)
				{
					return err;
				}
				targetBlock->runBefore[i] = run;
				zerosLeft -= run;
				i--;
			} while (zerosLeft != 0 && i != 0);			
		}
		else
		{
			run = 0;
		}
		targetBlock->runBefore[i] = zerosLeft;
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CResidual::parse_luma_residual_16x16_DC()
{
	int err = get_luma4x4_coeffs(LUMA_INTRA16x16DC, 0, 0);
	if (err < 0)
	{
		return err;
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CResidual::parse_chroma_residual(UINT8 cbp_chroma)
{
	int err = 0;
	// chroma DC
	for (int idx = 0; idx < 2; idx++)
	{
		if (m_macroblock_belongs->Get_pps_active()->Get_transform_8x8_mode_flag() == false)
		{
			// CAVLC
			err = get_chroma_DC_coeffs(idx);
			if (err < 0)
			{
				return err;
			}
		}
		else
		{
			// CABAC
		}
	}

	if (cbp_chroma & 2)
	{
		int err = 0;
		for (int component = 0; component < 2; component++)
		{
			for (int block_idx_y = 0; block_idx_y  < 2; block_idx_y++)
			{
				for (int block_idx_x = 0; block_idx_x < 2; block_idx_x++)
				{
					err = get_chroma_AC_coeffs(component, block_idx_x, block_idx_y);
					if (err < 0)
					{
						return err;
					}
				}
			}
		}
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CResidual::get_chroma_DC_coeffs(int chroma_idx)
{
	int err = 0;
	int max_coeff_num = 4, token = 0;
	int suffixLength = 0;
	
	UINT8 numCoeff = 0, trailingOnes = 0;
	err = get_numCoeff_and_trailingOnes_chromaDC(numCoeff, trailingOnes, token);
	if (err < 0)
	{
		return err;
	}
	else
	{
		chroma_DC_residual[chroma_idx].coeffToken = token;
		chroma_DC_residual[chroma_idx].numCoeff = numCoeff;
		chroma_DC_residual[chroma_idx].trailingOnes = trailingOnes;
	}

	if (numCoeff) //包含非0系数
	{
		if (trailingOnes) //拖尾系数
		{
			//读取拖尾系数符号
			int signValue = Get_uint_code_num(m_pSODB, m_bypeOffset, m_bitOffset, trailingOnes);
			int trailingCnt = trailingOnes;
			for (int coeffIdx = 0; coeffIdx < trailingOnes; coeffIdx++)
			{
				trailingCnt--;
				if ((signValue >> trailingCnt) & 1)
				{
					chroma_DC_residual[chroma_idx].trailingSign[coeffIdx] = -1;
				}
				else
				{
					chroma_DC_residual[chroma_idx].trailingSign[coeffIdx] = 1;
				}
			}
		}

		//读取解析level值
		int level = 0;
		if (numCoeff > 10 && trailingOnes < 3)
		{
			//根据上下文初始化suffixLength
			suffixLength = 1;
		}
		for (int k = 0; k <= numCoeff - 1 - trailingOnes; k++)
		{
			err = get_coeff_level(level, k, trailingOnes, suffixLength);
			if (err < 0)
			{
				return err;
			}

			if (suffixLength == 0)
			{
				suffixLength = 1;
			}

			if ((abs(level) >(3 << (suffixLength - 1))) && (suffixLength < 6))
			{
				suffixLength++;
			}

			chroma_DC_residual[chroma_idx].levels[k] = level;
		}


		// 读取解析run
		UINT8 zerosLeft = 0, totalZeros = 0, run = 0;
		if (numCoeff < max_coeff_num)
		{
			err = get_total_zeros_chromaDC(totalZeros, numCoeff - 1);
			if (err < 0)
			{
				return err;
			}
		}
		else
		{
			totalZeros = 0;
		}
		chroma_DC_residual[chroma_idx].totalZeros = totalZeros;

		//读取解析run_before
		int runBefore_vlcIdx = 0;
		int i = numCoeff - 1;
		zerosLeft = totalZeros;
		if (zerosLeft > 0 && i > 0)
		{
			do
			{
				runBefore_vlcIdx = (zerosLeft - 1 < 6 ? zerosLeft - 1 : 6);
				err = get_run_before(run, runBefore_vlcIdx);
				if (err < 0)
				{
					return err;
				}
				chroma_DC_residual[chroma_idx].runBefore[i] = run;
				zerosLeft -= run;
				i--;
			} while (zerosLeft != 0 && i != 0);
		}
		else
		{
			run = 0;
		}
		chroma_DC_residual[chroma_idx].runBefore[i] = zerosLeft;
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CResidual::get_chroma_AC_coeffs(int chroma_idx, int block_idc_row, int block_idc_column)
{
	int err = 0;
	int max_coeff_num = 15,i = 0;
	int numCoeff_vlcIdx = 0, prefixLength = 0, suffixLength = 0, level_prefix = 0, level_suffix = 0;

	int numberCurrent = m_macroblock_belongs->Get_number_current_chroma(chroma_idx, block_idc_row, block_idc_column);
	if (numberCurrent < 2)
	{
		numCoeff_vlcIdx = 0;
	}
	else if (numberCurrent < 4)
	{
		numCoeff_vlcIdx = 1;
	}
	else if (numberCurrent < 8)
	{
		numCoeff_vlcIdx = 2;
	}
	else
	{
		numCoeff_vlcIdx = 3;
	}

	// NumCoeff & TrailingOnes...
	UINT8 numCoeff = 0, trailingOnes = 0;
	int token = 0;
	err = get_numCoeff_and_trailingOnes(numCoeff, trailingOnes, token, numCoeff_vlcIdx);
	if (err < 0)
	{
		return err;
	}
	else
	{
		chroma_AC_residual[chroma_idx][block_idc_column][block_idc_row].coeffToken = token;
		chroma_AC_residual[chroma_idx][block_idc_column][block_idc_row].numCoeff = numCoeff;
		chroma_AC_residual[chroma_idx][block_idc_column][block_idc_row].trailingOnes = trailingOnes;
	}

	if (numCoeff) //包含非0系数
	{
		if (trailingOnes) //拖尾系数
		{
			//读取拖尾系数符号
			int signValue = Get_uint_code_num(m_pSODB, m_bypeOffset, m_bitOffset, trailingOnes);
			int trailingCnt = trailingOnes;
			for (int coeffIdx = 0; coeffIdx < trailingOnes; coeffIdx++)
			{
				trailingCnt--;
				if ((signValue >> trailingCnt) & 1)
				{
					chroma_AC_residual[chroma_idx][block_idc_column][block_idc_row].trailingSign[coeffIdx] = -1;
				}
				else
				{
					chroma_AC_residual[chroma_idx][block_idc_column][block_idc_row].trailingSign[coeffIdx] = 1;
				}
			}
		}

		//读取解析level值
		int level = 0;
		if (numCoeff > 10 && trailingOnes < 3)
		{
			//根据上下文初始化suffixLength
			suffixLength = 1;
		}
		for (int k = 0; k <= numCoeff - 1 - trailingOnes; k++)
		{
			err = get_coeff_level(level, k, trailingOnes, suffixLength);
			if (err < 0)
			{
				return err;
			}

			if (suffixLength == 0)
			{
				suffixLength = 1;
			}
						
			if ((abs(level) > (3 << (suffixLength - 1))) && (suffixLength < 6))
			{
				suffixLength++;
			}			

			chroma_AC_residual[chroma_idx][block_idc_column][block_idc_row].levels[k] = level;
		}

		// 读取解析run
		UINT8 zerosLeft = 0, totalZeros = 0, run = 0;
		if (numCoeff < max_coeff_num)
		{
			err = get_total_zeros(totalZeros, numCoeff - 1);
			if (err < 0)
			{
				return err;
			}
		}
		else
		{
			totalZeros = 0;
		}
		chroma_AC_residual[chroma_idx][block_idc_column][block_idc_row].totalZeros = totalZeros;

		//读取解析run_before
		int runBefore_vlcIdx = 0;
		i = numCoeff - 1;
		zerosLeft = totalZeros;
		if (zerosLeft > 0 && i > 0)
		{
			do
			{
				runBefore_vlcIdx = (zerosLeft - 1 < 6 ? zerosLeft - 1 : 6);
				err = get_run_before(run, runBefore_vlcIdx);
				if (err < 0)
				{
					return err;
				}
				chroma_AC_residual[chroma_idx][block_idc_column][block_idc_row].runBefore[i] = run;
				zerosLeft -= run;
				i--;
			} while (zerosLeft != 0 && i != 0);
		}
		else
		{
			run = 0;
		}
		chroma_AC_residual[chroma_idx][block_idc_column][block_idc_row].runBefore[i] = zerosLeft;
	}
	return kPARSING_ERROR_NO_ERROR;
}

int CResidual::get_numCoeff_and_trailingOnes(UINT8 &totalCoeff, UINT8 &trailingOnes, int &token, int numCoeff_vlcIdx)
{
	int err = 0;
	int *lengthTable = NULL, *codeTable = NULL;

	if (numCoeff_vlcIdx < 3)
	{
		lengthTable = &coeffTokenTable_Length[numCoeff_vlcIdx][0][0];
		codeTable = &coeffTokenTable_Code[numCoeff_vlcIdx][0][0];
		err = search_for_value_in_2D_table(totalCoeff, trailingOnes, token, lengthTable, codeTable, 17, 4);
		if (err < 0)
		{
			return err;
		}
	}
	else
	{
		totalCoeff = Get_uint_code_num(m_pSODB, m_bypeOffset, m_bitOffset, 4);
		trailingOnes = Get_uint_code_num(m_pSODB, m_bypeOffset, m_bitOffset, 2);
		if (!totalCoeff && trailingOnes == 3)
		{
			trailingOnes = 0;
		} 
		else
		{
			totalCoeff++;
		}
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CResidual::get_coeff_level(int &level, int levelIdx, UINT8 trailingOnes, int suffixLength)
{
	int prefixLength = 0, level_prefix = 0, level_suffix = 0;
	int levelSuffixSize = 0, levelCode = 0, i = 0;

	while (!Get_bit_at_position(m_pSODB, m_bypeOffset, m_bitOffset))
	{
		level_prefix++;
	}
	prefixLength = level_prefix + 1;
	if (level_prefix == 14 && suffixLength == 0)
	{
		levelSuffixSize = 4;
	}
	else if (level_prefix == 15)
	{
		levelSuffixSize = level_prefix - 3;
	}
	else
	{
		levelSuffixSize = suffixLength;
	}
	if (levelSuffixSize > 0)
	{
		level_suffix = Get_uint_code_num(m_pSODB, m_bypeOffset, m_bitOffset, levelSuffixSize);
	}
	else
	{
		level_suffix = 0;
	}
	levelCode = (min(15, level_prefix) << suffixLength) + level_suffix;
	if (level_prefix >= 15 && suffixLength == 0)
	{
		levelCode += 15;
	}
	if (level_prefix >= 16)
	{
		levelCode += (1 << (level_prefix - 3)) - 4096;
	}
	if (levelIdx == 0 && trailingOnes < 3)
	{
		levelCode += 2;
	}

	if (levelCode % 2 == 0)
	{
		level = (levelCode + 2) >> 1;
	}
	else
	{
		level = (-levelCode - 1) >> 1;
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CResidual::get_total_zeros(UINT8 &totalZeros, int totalZeros_vlcIdx)
{
	int err = 0, idx2 = 0;
	UINT8 idx1 = 0;
	int *lengthTable = &totalZerosTable_Length[totalZeros_vlcIdx][0];
	int *codeTable = &totalZerosTable_Code[totalZeros_vlcIdx][0];
	err = search_for_value_in_2D_table(totalZeros, idx1, idx2, lengthTable, codeTable, 16, 1);
	if (err < 0)
	{
		return err;
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CResidual::get_run_before(UINT8 &runBefore, int runBefore_vlcIdx)
{	
	UINT8 idx1 = 0;
	int idx2 = 0, err = 0;
	int *lengthTable = &runBeforeTable_Length[runBefore_vlcIdx][0];
	int *codeTable = &runBeforeTable_Code[runBefore_vlcIdx][0];
	err = search_for_value_in_2D_table(runBefore, idx1, idx2, lengthTable, codeTable, 16, 1);
	if (err < 0)
	{
		return err;
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CResidual::get_numCoeff_and_trailingOnes_chromaDC(UINT8 &totalCoeff, UINT8 &trailingOnes, int &token)
{
	int err = 0;
	int *lengthTable = &coeffTokenTableChromaDC_Length[0][0], *codeTable = &coeffTokenTableChromaDC_Code[0][0];

	err = search_for_value_in_2D_table(totalCoeff, trailingOnes, token, lengthTable, codeTable, 5, 4);
	if (err < 0)
	{
		return err;
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CResidual::get_total_zeros_chromaDC(UINT8 &totalZeros, int totalZeros_vlcIdx)
{
	int err = 0, idx2 = 0;
	UINT8 idx1 = 0;
	int *lengthTable = &totalZerosTableChromaDC_Length[totalZeros_vlcIdx][0];
	int *codeTable = &totalZerosTableChromaDC_Code[totalZeros_vlcIdx][0];
	err = search_for_value_in_2D_table(totalZeros, idx1, idx2, lengthTable, codeTable, 16, 1);
	if (err < 0)
	{
		return err;
	}

	return kPARSING_ERROR_NO_ERROR;
}

int CResidual::search_for_value_in_2D_table(UINT8 &value1, UINT8 &value2, int &code, int *lengthTable, int *codeTable, int tableWidth, int tableHeight)
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

void CResidual::restore_8x8_coeff_block_luma(int (*matrix)[4][4], int idx, int blockType)
{
	int blkColumnIdxStart = 2 * (idx / 2),  blkRowIdxStart = 2 * (idx % 2);
	for (int columnIdx = blkColumnIdxStart; columnIdx < blkColumnIdxStart + 2; columnIdx ++)
	{
		for (int rowIdx = blkRowIdxStart; rowIdx < blkRowIdxStart + 2; rowIdx++)
		{
			restore_4x4_coeff_block_luma(columnIdx, rowIdx, blockType);
		}
	}
}

void CResidual::restore_4x4_coeff_block_luma(int column_idx, int row_idx, int blockType)
{
	int max_coeff_num = 0, start_idx = 0;
	Coeff4x4Block *targetBlock = NULL;
	switch (blockType)
	{
	case LUMA:
		max_coeff_num = 16;
		targetBlock = &luma_residual[column_idx][row_idx];
		break;
	case LUMA_INTRA16x16AC:
		start_idx = 1;
		max_coeff_num = 15;
		targetBlock = &luma_residual16x16_AC[column_idx][row_idx];
		break;
	default:
		break;
	}

	int coeffBuf[16] = { 0 };
	UINT8 numCoeff = targetBlock->numCoeff, coeffIdx = numCoeff;
	UINT8 trailingOnes = targetBlock->trailingOnes, trailingLeft = trailingOnes;
	UINT8 totalZeros = targetBlock->totalZeros;

	// write trailing ones
	for (int i = numCoeff - trailingOnes, j = trailingOnes - 1; j >= 0; j--)
	{
		coeffBuf[i++] = targetBlock->trailingSign[j];
	}

	// write levels
	for (int i = numCoeff - trailingOnes - 1; i >= 0; i--)
	{
		coeffBuf[i] = targetBlock->levels[numCoeff - trailingOnes - 1 - i];
	}

	// move levels with run_before
	for (int i = numCoeff - 1; i > 0; i--)
	{
		swap(coeffBuf[i], coeffBuf[i + totalZeros]);
		totalZeros -= targetBlock->runBefore[i];
	}
	swap(coeffBuf[0], coeffBuf[totalZeros]);

	insert_matrix(m_coeff_matrix_luma, coeffBuf, start_idx, max_coeff_num, column_idx, row_idx);

// 	int blkIdx = position_to_block_index(row_idx, column_idx);
// 	coeff_invers_transform(m_coeff_matrix_luma[blkIdx], m_residual_matrix_luma[blkIdx]);
}

void CResidual::restore_8x8_coeff_block_chroma_DC(int(*matrix)[4][4][4], int idx)
{
	UINT8 numCoeff = chroma_DC_residual[idx].numCoeff;
	UINT8 trailingOnes = chroma_DC_residual[idx].trailingOnes, trailingLeft = trailingOnes;
	UINT8 totalZeros = chroma_DC_residual[idx].totalZeros;
	int coeffBuf[4] = { 0 };

	// write trailing ones
	for (int i = numCoeff - 1, j = trailingOnes - 1; j >= 0; j--)
	{
		coeffBuf[i--] = chroma_DC_residual[idx].trailingSign[j];
	}

	// write levels
	for (int i = numCoeff - trailingOnes - 1; i >= 0; i--)
	{
		coeffBuf[i] = chroma_DC_residual[idx].levels[numCoeff - trailingOnes - 1 - i];
	}

	// move levels with run_before
	for (int i = numCoeff - 1; i > 0; i--)
	{
		swap(coeffBuf[i], coeffBuf[i + totalZeros]);
		totalZeros -= chroma_DC_residual[idx].runBefore[i];
	}

	// insert coeffBuf......
	for (int pos = 0; pos < 4; pos++)
	{
		m_coeff_matrix_chroma[idx][pos][0][0] = coeffBuf[pos];
	}
}

void CResidual::restore_8x8_coeff_block_chroma_AC(int(*matrix)[4][4][4], int idx)
{
	int coeffBuf[15] = { 0 };
	for (int rowIdx = 0; rowIdx < 2; rowIdx++)
	{
		for (int columnIdx = 0; columnIdx < 2; columnIdx++)
		{
			UINT8 numCoeff = chroma_AC_residual[idx][rowIdx][columnIdx].numCoeff;
			UINT8 trailingOnes = chroma_AC_residual[idx][rowIdx][columnIdx].trailingOnes, trailingLeft = trailingOnes;
			UINT8 totalZeros = chroma_AC_residual[idx][rowIdx][columnIdx].totalZeros;

			// write trailing ones
			for (int i = numCoeff - 1, j = trailingOnes - 1; j >= 0; j--)
			{
				coeffBuf[i--] = chroma_AC_residual[idx][rowIdx][columnIdx].trailingSign[j];
			}

			// write levels
			for (int i = numCoeff - trailingOnes - 1; i >= 0; i--)
			{
				coeffBuf[i] = chroma_AC_residual[idx][rowIdx][columnIdx].levels[numCoeff - trailingOnes - 1 - i];
			}

			// move levels with run_before
			for (int i = numCoeff - 1; i > 0; i--)
			{
				swap(coeffBuf[i], coeffBuf[i + totalZeros]);
				totalZeros -= chroma_AC_residual[idx][rowIdx][columnIdx].runBefore[i];
			}

			// insert coeffBuf......
			insert_matrix(matrix[idx], coeffBuf, 1, 15, columnIdx, rowIdx);
		}
	}
}

void CResidual::restore_16x16_coeff_block_luma_DC(int(*matrix)[4][4])
{
	UINT8 numCoeff = luma_residual16x16_DC.numCoeff;
	UINT8 trailingOnes = luma_residual16x16_DC.trailingOnes, trailingLeft = trailingOnes;
	UINT8 totalZeros = luma_residual16x16_DC.totalZeros;
	int coeffBuf[16] = { 0 }, coeff_block[4][4] = {{ 0 }};

	// write trailing ones
	for (int i = numCoeff - trailingOnes, j = trailingOnes - 1; j >= 0; j--)
	{
		coeffBuf[i++] = luma_residual16x16_DC.trailingSign[j];
	}

	// write levels
	for (int i = numCoeff - trailingOnes - 1; i >= 0; i--)
	{
		coeffBuf[i] = luma_residual16x16_DC.levels[numCoeff - trailingOnes - 1 - i];
	}

	// move levels with run_before
	for (int i = numCoeff - 1; i > 0; i--)
	{
		swap(coeffBuf[i], coeffBuf[i + totalZeros]);
		totalZeros -= luma_residual16x16_DC.runBefore[i];
	}
	swap(coeffBuf[0], coeffBuf[totalZeros]);

	for (int pos = 0; pos < 16; pos++)
	{
		int row = SNGL_SCAN[pos][0], col = SNGL_SCAN[pos][1];
		coeff_block[col][row] = coeffBuf[pos];
	}

// 	printf("\nBefore DC Transform:\n");
// 	for (int col = 0; col < 4; col++)
// 	{
// 		for (int row = 0; row < 4; row++)
// 		{
// 			printf("%5d", coeff_block[col][row]);
// 		}
// 		printf("\n");
// 	}

	coeff_invers_DC_coeff(coeff_block);

// 	printf("\nAfter DC Transform:\n");
// 	for (int col = 0; col < 4; col++)
// 	{
// 		for (int row = 0; row < 4; row++)
// 		{
// 			printf("%5d", coeff_block[col][row]);
// 		}
// 		printf("\n");
// 	}

	// insert coeffBuf......
	for (int col = 0; col < 4; col++)
	{
		for (int row = 0; row < 4; row++)
		{
			int idx = position_to_block_index(row, col);
			m_coeff_matrix_luma[idx][0][0] = coeff_block[col][row];
		}
	}
}

void CResidual::insert_matrix(int(*matrix)[4][4], int *block, int start, int maxCoeffNum, int c, int r)
{
	int qp_per = m_qp / 6, qp_rem = m_qp % 6;
	int row = 0, column = 0;
	int coeff_buf[4][4] = { 0 }, residual_buf[4][4] = { 0 };
	int block_index = position_to_block_index(r, c);
	for (int idc = 0, pos0 = start; idc < maxCoeffNum && pos0 < 16; idc++)
	{
		row = SNGL_SCAN[pos0][0];
		column = SNGL_SCAN[pos0][1];
		matrix[block_index][column][row] = block[idc] * dequant_coef[qp_rem][row][column] << qp_per;
		pos0++;
	}
}

void CResidual::coeff_invers_transform(int(*coeff_buf)[4], int(*residual_buf)[4])
{
	int temp1[4] = { 0 }, temp2[4] = { 0 };

	// horizontal-trans
	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 4; i++)
		{
			temp1[i] = coeff_buf[i][j];
		}

		temp2[0] = (temp1[0] + temp1[2]);
		temp2[1] = (temp1[0] - temp1[2]);
		temp2[2] = (temp1[1] >> 1) - temp1[3];
		temp2[3] = temp1[1] + (temp1[3] >> 1);

		for (int i = 0; i < 2; i++)
		{
			int i1 = 3 - i;
			residual_buf[i][j] = temp2[i] + temp2[i1];
			residual_buf[i1][j] = temp2[i] - temp2[i1];
		}
	}

	// vertical-trans
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			temp1[j] = residual_buf[i][j];
		}

		temp2[0] = (temp1[0] + temp1[2]);
		temp2[1] = (temp1[0] - temp1[2]);
		temp2[2] = (temp1[1] >> 1) - temp1[3];
		temp2[3] = temp1[1] + (temp1[3] >> 1);

		for (int j = 0; j < 2; j++)
		{
			int j1 = 3 - j;
			residual_buf[i][j] = temp2[j] + temp2[j1];
			residual_buf[i][j1] = temp2[j] - temp2[j1];
		}
	}
}

void CResidual::coeff_invers_DC_coeff(int(*coeff_buf)[4])
{
	int M5[4] = { 0 };
	int M6[4] = { 0 };
	int qp_per = (m_qp - MIN_QP) / 6;
	int qp_rem = (m_qp - MIN_QP) % 6;
	int row = 0, col = 0;

	// horizontal
	for (col = 0; col < 4; col++)
	{
		for (row = 0; row < 4; row++)
		{
			M5[row] = coeff_buf[col][row];
		}

		M6[0] = M5[0] + M5[2];
		M6[1] = M5[0] - M5[2];
		M6[2] = M5[1] - M5[3];
		M6[3] = M5[1] + M5[3];

		for (row = 0; row < 2; row++)
		{
			int wor = 3 - row;
			coeff_buf[col][row] = M6[row] + M6[wor];
			coeff_buf[col][wor] = M6[row] - M6[wor];
		}
	}

	// vertical
	for (row = 0; row < 4; row++)
	{
		for (col = 0; col < 4; col++)
		{
			M5[col] = coeff_buf[col][row];
		}

		M6[0] = M5[0] + M5[2];
		M6[1] = M5[0] - M5[2];
		M6[2] = M5[1] - M5[3];
		M6[3] = M5[1] + M5[3];

		for (col = 0; col < 2; col++)
		{
			int loc = 3 - col;
			coeff_buf[col][row] = M6[col] + M6[loc];
			coeff_buf[loc][row] = M6[col] - M6[loc];
		}
	}

	for (col = 0; col < 4; col++)
	{
		for (row = 0; row < 4; row++)
		{
			coeff_buf[col][row] = ((coeff_buf[col][row] * dequant_coef[qp_rem][0][0] << qp_per) + 2) >> 2;
		}
	}
}

