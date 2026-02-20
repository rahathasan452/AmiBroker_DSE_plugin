// CMAE.h : main header file for the CMAE DLL
//

#if !defined(AFX_CMAE_H__580149FC_A0ED_4B12_9C3D_348579CE5B89__INCLUDED_)
#define AFX_CMAE_H__580149FC_A0ED_4B12_9C3D_348579CE5B89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CCMAEApp
// See CMAE.cpp for the implementation of this class
//

class CCMAEApp : public CWinApp
{
public:
	CCMAEApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCMAEApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CCMAEApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CMAE_H__580149FC_A0ED_4B12_9C3D_348579CE5B89__INCLUDED_)
