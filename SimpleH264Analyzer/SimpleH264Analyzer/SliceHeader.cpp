#include "stdafx.h"
#include "SliceHeader.h"
#include "SeqParamSet.h"
#include "PicParamSet.h"

using namespace std;

CSliceHeader::CSliceHeader(UINT8 *pSODB, CSeqParamSet *sps, CPicParamSet *pps, UINT8 nalType)
{
	m_pSODB = pSODB;
	m_sps_active = sps;
	m_pps_active = pps;
	m_nalType = nalType;

	m_first_mb_in_slice = 0;
	m_slice_type = 0;
	m_pps_id = 0;
	m_colour_plane_id = 0;
	m_frame_num = 0;
	m_field_pic_flag = 0;
	m_bottom_field_flag = 0;
	m_idr_pic_id = 0;
	m_poc = 0;
	m_delta_poc_bottom = 0;
	m_dec_ref_pic_marking = { 0, 0 };
	m_slice_qp_delta = 0;
}

CSliceHeader::~CSliceHeader()
{
}

UINT32 CSliceHeader::Parse_slice_header()
{
	UINT32 sliceHeaderLengthInBits = 0;
	UINT8  bitPosition = 0;
	UINT32 bytePosition = 0;

	m_first_mb_in_slice = Get_uev_code_num(m_pSODB, bytePosition, bitPosition);
	m_slice_type = Get_uev_code_num(m_pSODB, bytePosition, bitPosition);
	m_slice_type %= 5;
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
	
	if (m_nalType == 5)
	{
		m_dec_ref_pic_marking.no_output_of_prior_pics_flag = Get_bit_at_position(m_pSODB, bytePosition, bitPosition);
		m_dec_ref_pic_marking.long_term_reference_flag = Get_bit_at_position(m_pSODB, bytePosition, bitPosition);
	}

	m_slice_qp_delta = Get_sev_code_num(m_pSODB, bytePosition, bitPosition);

	sliceHeaderLengthInBits = 8 * bytePosition + bitPosition;

	return sliceHeaderLengthInBits;
}

void CSliceHeader::Dump_slice_header_info()
{
#if TRACE_CONFIG_SLICE_HEADER

#if TRACE_CONFIG_LOGOUT
	g_traceFile << "--------------- Slice Header ----------------" << endl;
	g_traceFile << "First MB In Slice: " << to_string(m_first_mb_in_slice) << endl;
	g_traceFile << "Slice Type: " << to_string(m_slice_type) << endl;
	g_traceFile << "Picture Parameter Set ID: " << to_string(m_pps_id) << endl;
	if (m_sps_active->Get_separate_colour_plane_flag())
	{
		g_traceFile << "Color Plane ID: " << to_string(m_colour_plane_id) << endl;
	}
	g_traceFile << "Frame Num: " << to_string(m_frame_num) << endl;
	if (!m_sps_active->Get_frame_mbs_only_flag())
	{
		g_traceFile << "field_pic_flag: " << to_string(m_field_pic_flag) << endl;
		if (m_field_pic_flag)
		{
			g_traceFile << "bottom_field_flag: " << to_string(m_bottom_field_flag) << endl;
		}
	}

	if (m_nalType == 5)
	{
		g_traceFile << "IDR Picture Flag: " << to_string(m_idr_pic_id) << endl;
	}

	if (m_sps_active->Get_poc_type() == 0)
	{
		g_traceFile << "Picture Order Count: " << to_string(m_poc) << endl;
		if ((!m_field_pic_flag) && m_pps_active->Get_bottom_field_pic_order_in_frame_present_flag())
		{
			g_traceFile << "delta_pic_order_cnt_bottom: " << to_string(m_delta_poc_bottom) << endl;
		}
	}

	if (m_nalType == 5)
	{
		g_traceFile << "output_of_prior_pics_flag: " << to_string(m_dec_ref_pic_marking.no_output_of_prior_pics_flag) << endl;
		g_traceFile << "long_term_reference_flag: " << to_string(m_dec_ref_pic_marking.long_term_reference_flag) << endl;
	}

	g_traceFile << "Slice QP Delta: " << to_string(m_slice_qp_delta) << endl;
	g_traceFile << "-------------------------------------------" << endl;
#endif

#endif
}

UINT8 CSliceHeader::Get_slice_type()
{
	return m_slice_type;
}

int CSliceHeader::Get_slice_qp_delta()
{
	return m_slice_qp_delta;
}
