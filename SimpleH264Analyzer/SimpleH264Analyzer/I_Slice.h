#ifndef _I_SLICE_H_
#define _I_SLICE_H_

class CSeqParamSet;
class CPicParamSet;
class CSliceHeader;

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
};


#endif