// ASCII.h : main header file for the ASCII DLL
//

#if !defined(AFX_ASCII_H__3EA38A65_0F4A_460F_A028_2597B8826ED6__INCLUDED_)
#define AFX_ASCII_H__3EA38A65_0F4A_460F_A028_2597B8826ED6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CASCIIApp
// See ASCII.cpp for the implementation of this class
//

class CASCIIApp : public CWinApp
{
public:
	CASCIIApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CASCIIApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CASCIIApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ASCII_H__3EA38A65_0F4A_460F_A028_2597B8826ED6__INCLUDED_)
