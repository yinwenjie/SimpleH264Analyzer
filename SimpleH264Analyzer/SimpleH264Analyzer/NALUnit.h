#pragma once
class CNALUnit
{
public:
	CNALUnit(uint8	*pSODB, uint32	SODBLength);
	~CNALUnit();

private:
	uint8	*m_pSODB;
	uint32	m_SODBLength;
};

