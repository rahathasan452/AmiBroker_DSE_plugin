#if !defined(AFX_SRECORDSET_H__BEFC2AAB_5AEA_4AC9_A473_980EF0DC98D0__INCLUDED_)
#define AFX_SRECORDSET_H__BEFC2AAB_5AEA_4AC9_A473_980EF0DC98D0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SRecordset.h : header file
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
/////////////////////////////////////////////////////////////////////////////
// CSRecordset recordset

class CSRecordset : public CRecordset
{
public:
	DECLARE_DYNAMIC(CSRecordset)

	CSRecordset( CDatabase *pDatabase );
// Field/Param Data
	//{{AFX_FIELD(CSRecordset, CRecordset)
	//}}AFX_FIELD


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSRecordset)
	public:
	virtual BOOL Open(UINT nOpenType = snapshot, LPCTSTR lpszSQL = NULL, DWORD dwOptions = none);
	//}}AFX_VIRTUAL

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SRECORDSET_H__BEFC2AAB_5AEA_4AC9_A473_980EF0DC98D0__INCLUDED_)
