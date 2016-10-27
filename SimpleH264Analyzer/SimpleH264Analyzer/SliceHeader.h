#ifndef _SLICE_HEADER_H_
#define _SLICE_HEADER_H_

class CSeqParamSet;
class CPicParamSet;

class CSliceHeader
{
public:
	CSliceHeader(UINT8	*pSODB, CSeqParamSet *sps, CPicParamSet *pps, UINT8	nalType);
	~CSliceHeader();

	UINT32 Parse_slice_header();

private:
	CSeqParamSet *m_sps_active;
	CPicParamSet *m_pps_active;
	UINT8	*m_pSODB;
	UINT8   m_nalType;

	UINT16 m_first_mb_in_slice;
	UINT8  m_slice_type;
	UINT8  m_pps_id;
	UINT8  m_colour_plane_id;
	UINT32 m_frame_num;
	bool   m_field_pic_flag;
	bool   m_bottom_field_flag;
	UINT16 m_idr_pic_id;
	UINT32 m_poc;
	int	   m_delta_poc_bottom;
};

#endif