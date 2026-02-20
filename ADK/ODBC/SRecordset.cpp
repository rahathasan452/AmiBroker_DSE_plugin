// SRecordset.cpp : implementation file
//
// Copyright (C)2001-2006 Tomasz Janeczko, amibroker.com
// All rights reserved.
//
// Last modified: 2006-07-21 TJ
// 
// You may use this code in your own projects provided that:
//
// 1. You are registered user of AmiBroker
// 2. The software you write using it is for personal, noncommercial use only
//
// For commercial use you have to obtain a separate license from Amibroker.com
//
////////////////////////////////////////////////////
#include "stdafx.h"
#include "ODBC.h"
#include "SRecordset.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSRecordset

IMPLEMENT_DYNAMIC(CSRecordset, CRecordset)

CSRecordset::CSRecordset( CDatabase *pDatabase ) : CRecordset( pDatabase )
{
}


BOOL CSRecordset::Open(UINT nOpenType, LPCTSTR lpszSQL, DWORD dwOptions) 
{
	ASSERT(!IsOpen());
	ASSERT_VALID(this);
	ASSERT(lpszSQL == NULL || AfxIsValidString(lpszSQL));
	ASSERT(nOpenType == AFX_DB_USE_DEFAULT_TYPE ||
		nOpenType == dynaset || nOpenType == snapshot ||
		nOpenType == forwardOnly || nOpenType == dynamic);
	ASSERT(!(dwOptions & readOnly && dwOptions & appendOnly));

	// Can only use optimizeBulkAdd with appendOnly recordsets
	ASSERT((dwOptions & optimizeBulkAdd && dwOptions & appendOnly) ||
		!(dwOptions & optimizeBulkAdd));

	// forwardOnly recordsets have limited functionality
	ASSERT(!(nOpenType == forwardOnly && dwOptions & skipDeletedRecords));

	// Cache state info and allocate hstmt
	SetState(nOpenType, lpszSQL, dwOptions);
	if(!AllocHstmt())
		return FALSE;

	// Check if bookmarks upported (CanBookmark depends on open DB)
	ASSERT(dwOptions & useBookmarks ? CanBookmark() : TRUE);

	TRY
	{
		OnSetOptions(m_hstmt);

		// Allocate the field/param status arrays, if necessary
		BOOL bUnbound = FALSE;
		if (m_nFields > 0 || m_nParams > 0)
			AllocStatusArrays();
		else
			bUnbound = TRUE;

		// ORIGINAL MFC builts SQL statement 
		// BUT WE DONT WANT THAT!
		// BuildSelectSQL();

		m_strSQL = lpszSQL;
		PrepareAndExecute();

		// Cache some field info and prepare the rowset
		AllocAndCacheFieldInfo();
		AllocRowset();

		// If late binding, still need to allocate status arrays
		if (bUnbound && (m_nFields > 0 || m_nParams > 0))
			AllocStatusArrays();

		// Give derived classes a call before binding
		PreBindFields();

		// Fetch the first row of data
		MoveNext();

		// If EOF, then result set empty, so set BOF as well
		m_bBOF = m_bEOF;
	}
	CATCH_ALL(e)
	{
		Close();
		THROW_LAST();
	}
	END_CATCH_ALL

	return TRUE;
}
