#include "stdafx.h"
#include "Stream.h"

#include <iostream>

using namespace std;

void FileInfo(TCHAR *pFileName)
{
	wcout << L"File name: " << pFileName << endl;
}

void ErrorInfo(int nIdx)
{
	switch (nIdx)
	{
	case 0:
		wcout << L"Illegal command line format." << endl;
		break;
	case 1:
		wcout << L"Opening bit stream file failed." << endl;
		break;
	default:
		break;
	}	
}

CStreamFile::CStreamFile(TCHAR *fileName)
{
	m_pFileName = fileName;
	if (m_pFileName)
	{
		FileInfo(m_pFileName);
		_tfopen_s(&m_pFile_In, m_pFileName, _T("rb"));
		if (NULL == m_pFile_In)
		{
			ErrorInfo(2);
		}
	}
}

CStreamFile::~CStreamFile()
{
	if (NULL != m_pFile_In)
	{
		fclose(m_pFile_In);
		m_pFile_In = NULL;
	}
}

int CStreamFile::Parse_h264_bitstream()
{
	int		ret = 0;
	ret = find_first_NAL_unit();		//Find position of 1st nal unit
	if (!ret)
	{
		return ret;
	}

	do 
	{
		ret = extract_nal_unit();
	} while (ret);

	return ret;
}

int CStreamFile::find_first_NAL_unit()
{
	int getStartCode = 0;
	while (! feof (m_pFile_In))
	{
		if (0x01 == peek_bytes(3))
		{
			//find start code  "00 00 01"
			for (int nIdx = 0; nIdx < 3; nIdx++)
			{
				getc(m_pFile_In);
			}
			getStartCode = 1;
			break;
		}
		else if (0x01 == peek_bytes(4))
		{
			//find start code  "00 00 00 01"
			for (int nIdx = 0; nIdx < 4; nIdx++)
			{
				getc(m_pFile_In);
			}
			getStartCode = 1;
			break;
		}
		getc(m_pFile_In);
	}

	if (getStartCode)
	{
		return 1;
	} 
	else
	{
		wcout << L"End of file reached." << endl;
		return 0;
	}
}

int CStreamFile::peek_bytes(int n)
{
	if (n > 4)
	{
		wcout << L"Error: Too many bytes to peek." << endl;
		return -3;
	}

	int nOut = 0;
	fpos_t pos;
	fgetpos (m_pFile_In,&pos);
	uint8 nBuf = 0;
	for (int nIdx = 0; nIdx < n; nIdx++)
	{
		nBuf = (uint8)getc(m_pFile_In);
		nOut = (nOut << 8) | nBuf;
	}
	fsetpos (m_pFile_In,&pos);
	return nOut;
}

int CStreamFile::extract_nal_unit()
{
	m_NalVec.clear();

	uint8 nThis = 0;
	while (! feof (m_pFile_In))
	{
		if (0x01 == peek_bytes(3))
		{
			//find start code  "00 00 01"
			for (int nIdx = 0; nIdx < 3; nIdx++)
			{
				getc(m_pFile_In);
			}
			return 1;
		}
		else if (0x01 == peek_bytes(4))
		{
			//find start code  "00 00 00 01"
			for (int nIdx = 0; nIdx < 4; nIdx++)
			{
				getc(m_pFile_In);
			}
			return 1;
		}
		else
		{
			// not start code
			m_NalVec.push_back((uint8)getc(m_pFile_In));
		}
	}

	wcout << L"End of file reached." << endl;
	return 0;
}
