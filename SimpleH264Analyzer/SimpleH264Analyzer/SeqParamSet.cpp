#include "stdafx.h"
#include "SeqParamSet.h"


CSeqParamSet::CSeqParamSet()
{
	memset(this, 0, sizeof(CSeqParamSet));
}


CSeqParamSet::~CSeqParamSet()
{
}

void CSeqParamSet::Set_profile_level_idc(UINT8 profile, UINT8 level)
{
	m_profile_idc = profile;
	m_level_idc = level;
}

void CSeqParamSet::Set_sps_id(UINT8 sps_id)
{
	m_sps_id = sps_id;
}

void CSeqParamSet::Set_max_frame_num(UINT32 maxFrameNum)
{
	m_max_frame_num = maxFrameNum;
}

void CSeqParamSet::Set_poc_type(UINT8 pocType)
{
	m_poc_type = pocType;
}

void CSeqParamSet::Set_max_poc_cnt(UINT32 maxPocCnt)
{
	m_max_poc_cnt = maxPocCnt;
}

void CSeqParamSet::Set_max_num_ref_frames(UINT32 maxRefFrames)
{
	m_max_num_ref_frames = maxRefFrames;
}

void CSeqParamSet::Set_sps_multiple_flags(UINT32 flags)
{
	m_separate_colour_plane_flag = flags & (1 << 21);
	m_qpprime_y_zero_transform_bypass_flag = flags & (1 << 20);
	m_seq_scaling_matrix_present_flag = flags & (1 << 19);

	m_gaps_in_frame_num_value_allowed_flag = flags & (1 << 5);
	m_frame_mbs_only_flag = flags & (1 << 4);
	m_mb_adaptive_frame_field_flag = flags & (1 << 3);
	m_direct_8x8_inference_flag = flags & (1 << 2);
	m_frame_cropping_flag = flags & (1 << 1);
	m_vui_parameters_present_flag = flags & 1;
}

void CSeqParamSet::Set_pic_reslution_in_mbs(UINT16 widthInMBs, UINT16 heightInMapUnits)
{
	m_pic_width_in_mbs = widthInMBs;
	m_pic_height_in_map_units = heightInMapUnits;
	m_pic_height_in_mbs = m_frame_mbs_only_flag ? m_pic_height_in_map_units : 2 * m_pic_height_in_map_units;
}

void CSeqParamSet::Set_frame_crop_offset(UINT32 offsets[4])
{
	for (int idx = 0; idx < 4; idx++)
	{
		m_frame_crop_offset[idx] = offsets[idx];
	}
}
