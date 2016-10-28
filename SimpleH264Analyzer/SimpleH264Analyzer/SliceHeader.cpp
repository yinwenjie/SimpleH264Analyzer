#include "stdafx.h"
#include "SliceHeader.h"
#include "SeqParamSet.h"
#include "PicParamSet.h"

CSliceHeader::CSliceHeader(UINT8 *pSODB, CSeqParamSet *sps, CPicParamSet *pps, UINT8 nalType)
{
	memset(this, 0, sizeof(CSliceHeader));
	m_pSODB = pSODB;
	m_sps_active = sps;
	m_pps_active = pps;
	m_nalType = nalType;
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

	if (!m_sps_active->Get_frame_mbs_only_flag())
	{
		m_field_pic_flag = Get_bit_at_position(m_pSODB, bytePosition, bitPosition);
		if (m_field_pic_flag)
		{
			m_bottom_field_flag = Get_bit_at_position(m_pSODB, bytePosition, bitPosition);
		}
	}

	if (m_nalType == 5)
	{
		m_idr_pic_id = Get_uev_code_num(m_pSODB, bytePosition, bitPosition);
	}

	if (m_sps_active->Get_poc_type() == 0)
	{
		m_poc = Get_uint_code_num(m_pSODB, bytePosition, bitPosition, m_sps_active->Get_log2_max_poc_cnt());
		if ((!m_field_pic_flag) && m_pps_active->Get_bottom_field_pic_order_in_frame_present_flag())
		{
			m_delta_poc_bottom = Get_sev_code_num(m_pSODB, bytePosition, bitPosition);
		}
	}
	
	return sliceHeaderLengthInBits;
}
