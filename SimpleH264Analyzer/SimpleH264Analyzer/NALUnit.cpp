#include "stdafx.h"
#include "NALUnit.h"


CNALUnit::CNALUnit(UINT8	*pSODB, UINT32	SODBLength, UINT8 nalType)
{
	m_pSODB = pSODB;
	m_SODBLength = SODBLength;

	m_nalType = nalType;
}


CNALUnit::~CNALUnit()
{
}
