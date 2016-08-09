#include "stdafx.h"
#include "NALUnit.h"


CNALUnit::CNALUnit(uint8	*pSODB, uint32	SODBLength)
{
	m_pSODB = pSODB;
	m_SODBLength = SODBLength;
}


CNALUnit::~CNALUnit()
{
}
