// ODBC.h : main header file for the ODBC DLL
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
#if !defined(AFX_ODBC_H__EE2EEABD_CAFA_4C4E_A253_08C35A34364B__INCLUDED_)
#define AFX_ODBC_H__EE2EEABD_CAFA_4C4E_A253_08C35A34364B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CODBCApp
// See ODBC.cpp for the implementation of this class
//

class CODBCApp : public CWinApp
{
public:
	CODBCApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CODBCApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CODBCApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ODBC_H__EE2EEABD_CAFA_4C4E_A253_08C35A34364B__INCLUDED_)
