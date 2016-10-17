#include "stdafx.h"
#include "PicParamSet.h"


CPicParamSet::CPicParamSet()
{
	memset(this, 0, sizeof(CPicParamSet));
}


CPicParamSet::~CPicParamSet()
{
}

void CPicParamSet::Set_pps_id(UINT8 ppsID)
{
	m_pps_id = ppsID;
}

void CPicParamSet::Set_sps_id(UINT8 spsID)
{
	m_sps_id = spsID;
}

void CPicParamSet::Set_num_slice_groups(UINT8 num_slice_grops)
{
	m_num_slice_groups = num_slice_grops;
}

void CPicParamSet::Set_num_ref_idx(UINT8 l0, UINT8 l1)
{
	m_num_ref_idx_l0_default_active = l0;
	m_num_ref_idx_l1_default_active = l1;
}

void CPicParamSet::Set_weighted_bipred_idc(UINT8 weighted_bipred_idc)
{
	m_weighted_bipred_idc = weighted_bipred_idc;
}

void CPicParamSet::Set_pic_init_qp(int pic_init_qp)
{
	m_pic_init_qp = pic_init_qp;
}

void CPicParamSet::Set_pic_init_qs(int  pic_init_qs)
{
	m_pic_init_qs = pic_init_qs;
}

void CPicParamSet::Set_chroma_qp_index_offset(int chroma_qp_index_offset)
{
	m_chroma_qp_index_offset = chroma_qp_index_offset;
}

void CPicParamSet::Set_multiple_flags(UINT16 flags)
{
	m_entropy_coding_flag = flags & 1;
	m_bottom_field_pic_order_in_frame_present_flag = flags & (1 << 1);
	m_weighted_pred_flag = flags & (1 << 2);
	m_deblocking_filter_control_present_flag = flags & (1 << 3);
	m_constrained_intra_pred_flag = flags & (1 << 4);
	m_redundant_pic_cnt_present_flag = flags & (1 << 5);
}
