#ifndef _I_SLICE_H_
#define _I_SLICE_H_

class CSeqParamSet;
class CPicParamSet;
class CSliceHeader;
class CMacroBlock;

class I_Slice
{
public:
	I_Slice(UINT8	*pSODB, CSeqParamSet *sps, CPicParamSet *pps, UINT8	nalType);
	~I_Slice();

	int Parse();

private:
	CSeqParamSet *m_sps_active;
	CPicParamSet *m_pps_active;
	UINT8	*m_pSODB;
	UINT8   m_nalType;

	CSliceHeader *m_sliceHeader;	

	UINT16 m_max_mb_number;
	CMacroBlock **m_macroblocks;
};


#endif