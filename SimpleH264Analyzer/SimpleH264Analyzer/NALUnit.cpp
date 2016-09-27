#include "stdafx.h"
#include "NALUnit.h"


CNALUnit::CNALUnit(UINT8	*pSODB, UINT32	SODBLength)
{
	m_pSODB = pSODB;
	m_SODBLength = SODBLength;
}


CNALUnit::~CNALUnit()
{
}
