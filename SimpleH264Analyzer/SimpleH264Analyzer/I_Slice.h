#ifndef _I_SLICE_H_
#define _I_SLICE_H_

class CNALUnit;
class CSeqParamSet;
class CPicParamSet;
class CSliceHeader;
class I_Slice
{
public:
	I_Slice(CNALUnit *pNALU, CSeqParamSet *sps, CPicParamSet *pps);
	~I_Slice();

	int Parse();

private:
	CSliceHeader *m_sliceHeader;
	CSeqParamSet *m_sps;
	CPicParamSet *m_pps;

	UINT8 *m_sliceBuffer;
};


#endif