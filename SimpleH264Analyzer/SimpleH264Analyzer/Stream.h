#ifndef _STREAM_H_
#define _STREAM_H_

#include <vector>

class CStreamFile
{
public:
	CStreamFile(TCHAR *fileName);
	~CStreamFile();
	
	//Open API
	int Parse_h264_bitstream();

private:
	FILE	*m_pFile_In;
	TCHAR	*m_pFileName;
	std::vector<uint8> m_NalVec;

	//Private methods
	int find_first_NAL_unit();
	int extract_nal_unit();
	int peek_bytes(int n);
};

#endif