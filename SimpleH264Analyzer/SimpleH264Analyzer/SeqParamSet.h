#ifndef _SEQ_PARAM_SET_H_
#define _SEQ_PARAM_SET_H_

class CSeqParamSet
{
public:
	CSeqParamSet();
	~CSeqParamSet();

private:
	UINT8  m_profile_idc;
	UINT8  m_level_idc;
	UINT8  m_sps_id;

	// for uncommon profile...
	UINT8  m_chroma_format_idc;
	bool   m_separate_colour_plane_flag;
	UINT8  m_bit_depth_luma;
	UINT8  m_bit_depth_chroma;
	bool   m_qpprime_y_zero_transform_bypass_flag;
	bool   m_seq_scaling_matrix_present_flag;
	// ...for uncommon profile

	UINT32 m_max_frame_num;
	UINT8  m_poc_type;
	UINT32 m_max_poc_cnt;
	UINT32 m_max_num_ref_frames;
	bool   m_gaps_in_frame_num_value_allowed_flag;
	UINT16 m_pic_width_in_mbs;
	UINT16 m_pic_height_in_map_units;
	UINT16 m_pic_height_in_mbs;	// not defined in spec, derived...
	bool   m_frame_mbs_only_flag;
	bool   m_mb_adaptive_frame_field_flag;
	bool   m_direct_8x8_inference_flag;
	bool   m_frame_cropping_flag;
	UINT32 m_frame_crop_offset;
	bool   m_vui_parameters_present_flag;
};

#endif

