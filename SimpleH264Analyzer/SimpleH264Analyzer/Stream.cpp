#include "stdafx.h"
#include "Stream.h"
#include "NALUnit.h"

#include <iostream>

using namespace std;

// 构造函数
CStreamFile::CStreamFile(TCHAR *fileName)
{
	m_fileName = fileName;
	file_info();
	_tfopen_s(&m_inputFile, m_fileName, _T("rb"));
	if (NULL ==  m_inputFile)
	{
		file_error(0);
	}

}

//析构函数
CStreamFile::~CStreamFile()
{
	if (NULL != m_inputFile)
	{
		fclose(m_inputFile);
		m_inputFile = NULL;
	}
}

void CStreamFile::file_info()
{
	if (m_fileName)
	{
		wcout << L"File name :" << m_fileName << endl;
	}
}

void CStreamFile::file_error(int idx)
{
	switch (idx)
	{
	case 0:
		wcout << L"Error: opening input file failed." << endl;
		break;
	default:
		break;
	}
}

int CStreamFile::Parse_h264_bitstream()
{
	int ret = 0;
	uint8 nalType = 0;
	do 
	{
		ret = find_nal_prefix();
		//解析NAL UNIT
		if (m_nalVec.size())
		{
			uint8 nalType = m_nalVec[0] & 0x1F;
			wcout << L"NAL Unit Type: " << nalType << endl;
			ebsp_to_sodb();
			CNALUnit nalUnit(&m_nalVec[1], m_nalVec.size() - 1);
		}
		
	} while (ret);
	return 0;
}

int CStreamFile::find_nal_prefix()
{
	uint8 prefix[3] = { 0 };
	uint8 fileByte;
	/*
	[0][1][2] = {0 0 0} -> [1][2][0] ={0 0 0} -> [2][0][1] = {0 0 0}
	getc() = 1 -> 0 0 0 1
	[0][1][2] = {0 0 1} -> [1][2][0] ={0 0 1} -> [2][0][1] = {0 0 1}
	*/

	m_nalVec.clear();

	int pos = 0, getPrefix = 0;
	for (int idx = 0; idx < 3; idx++)
	{
		prefix[idx] = getc(m_inputFile);
		m_nalVec.push_back(prefix[idx]);
	}

	while (!feof(m_inputFile))
	{
		if ((prefix[pos % 3] == 0) && (prefix[(pos + 1) % 3] == 0) && (prefix[(pos + 2) % 3] == 1))
		{
			//0x 00 00 01 found
			getPrefix = 1;
			m_nalVec.pop_back();
			m_nalVec.pop_back();
			m_nalVec.pop_back();
			break;
		}
		else if ((prefix[pos % 3] == 0) && (prefix[(pos + 1) % 3] == 0) && (prefix[(pos + 2) % 3] == 0))
		{
			if (1 == getc(m_inputFile))
			{
				//0x 00 00 00 01 found
				getPrefix = 2;
				m_nalVec.pop_back();
				m_nalVec.pop_back();
				m_nalVec.pop_back();
				break;
			}
		}
		else
		{
			fileByte = getc(m_inputFile);
			prefix[(pos++) % 3] = fileByte;
			m_nalVec.push_back(fileByte);
		}
	}

	return getPrefix;
}

void CStreamFile::ebsp_to_sodb()
{
	if (m_nalVec.size() < 3)
	{
		return;
	}

	for (vector<uint8>::iterator itor = m_nalVec.begin() + 2; itor != m_nalVec.end(); )
	{
		if ((3 == *itor) && (0 == *(itor-1)) && (0 == *(itor-2)))
		{
			vector<uint8>::iterator temp = m_nalVec.erase(itor);
			itor = temp;
		}
		else
		{
			itor++;
		}
	}
}


