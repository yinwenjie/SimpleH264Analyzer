#include "stdafx.h"
#include "I_Slice.h"
#include "NALUnit.h"
#include "SliceHeader.h"

I_Slice::I_Slice(CNALUnit *pNALU, CSeqParamSet *sps, CPicParamSet *pps)
{
	m_sliceHeader = NULL;
	m_sps = sps;
	m_pps = pps;
	m_sliceBuffer = pNALU->Get_slice_data_buffer();
}

I_Slice::~I_Slice()
{
}

int I_Slice::Parse()
{
	m_sliceHeader = new CSliceHeader(m_sliceBuffer, m_sps, m_pps, 5);
	m_sliceHeader->Parse_slice_header();

	return 0;
}
