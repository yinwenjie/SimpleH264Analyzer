#ifndef _STREAM_H_
#define _STREAM_H_

class CStreamFile
{
public:
	CStreamFile(TCHAR *fileName);
	~CStreamFile();
	
private:
	FILE	*m_pFile_In;
	TCHAR	*m_pFileName;
};

#endif