#pragma once
class CNALUnit
{
public:
	CNALUnit(UINT8	*pSODB, UINT32	SODBLength);
	~CNALUnit();

private:
	UINT8	*m_pSODB;
	UINT32	m_SODBLength;
};

