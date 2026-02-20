// Tribes.h : main header file for the Tribes DLL
//

#if !defined(AFX_Tribes_H__580149FC_A0ED_4B12_9C3D_348579CE5B89__INCLUDED_)
#define AFX_Tribes_H__580149FC_A0ED_4B12_9C3D_348579CE5B89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTribesApp
// See Tribes.cpp for the implementation of this class
//

class CTribesApp : public CWinApp
{
public:
	CTribesApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTribesApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CTribesApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_Tribes_H__580149FC_A0ED_4B12_9C3D_348579CE5B89__INCLUDED_)
