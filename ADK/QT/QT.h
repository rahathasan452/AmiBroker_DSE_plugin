// QT.h : main header file for the QT DLL
//

#if !defined(AFX_QT_H__728CE6DF_A647_40EF_8085_77A8FAE4E133__INCLUDED_)
#define AFX_QT_H__728CE6DF_A647_40EF_8085_77A8FAE4E133__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CQTApp
// See QT.cpp for the implementation of this class
//

class CQTApp : public CWinApp
{
public:
	CQTApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQTApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CQTApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QT_H__728CE6DF_A647_40EF_8085_77A8FAE4E133__INCLUDED_)
