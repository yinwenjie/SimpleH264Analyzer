#include "stdafx.h"
#include "SliceHeader.h"
#include "SeqParamSet.h"

CSliceHeader::CSliceHeader(UINT8 *pSODB, CSeqParamSet *sps, CPicParamSet *pps)
{
	m_pSODB = pSODB;
	m_sps_active = sps;
	m_pps_active = pps;
}

CSliceHeader::~CSliceHeader()
{
}

UINT32 CSliceHeader::Parse_slice_header()
{
	UINT32 sliceHeaderLengthInBits = 0;
	UINT8 bytePosition = 0, bitPosition = 0;

	m_first_mb_in_slice = Get_uev_code_num(m_pSODB, bytePosition, bitPosition);
	m_slice_type = Get_uev_code_num(m_pSODB, bytePosition, bitPosition);
	m_pps_id = Get_uev_code_num(m_pSODB, bytePosition, bitPosition);

	if (m_sps_active->Get_separate_colour_plane_flag())
	{
		m_colour_plane_id = Get_uint_code_num(m_pSODB, bytePosition, bitPosition, 2);
	}

	m_frame_num = Get_uint_code_num(m_pSODB, bytePosition, bitPosition, m_sps_active->Get_log2_max_frame_num());

	return sliceHeaderLengthInBits;
}
