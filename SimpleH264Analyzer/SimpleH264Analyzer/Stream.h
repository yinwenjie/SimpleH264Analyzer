#ifndef _STREAM_H_
#define _STREAM_H_

#include <vector>

class CSeqParamSet;
class CPicParamSet;
class I_Slice;

class CStreamFile
{
public:
	CStreamFile(TCHAR *fileName);
	~CStreamFile();

	//Open API
	int Parse_h264_bitstream();
	void Dump_NAL_type(UINT8 nalType);

private:
	FILE	*m_inputFile;
	TCHAR	*m_fileName;
	std::vector<UINT8> m_nalVec;
	std::vector<I_Slice> m_IDRVec;
	
	void	file_info();
	void	file_error(int idx);
	int		find_nal_prefix();
	void	ebsp_to_sodb();

	CSeqParamSet *m_sps;
	CPicParamSet *m_pps;
	I_Slice *m_IDRSlice;
};


#endif