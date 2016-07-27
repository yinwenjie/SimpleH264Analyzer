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
