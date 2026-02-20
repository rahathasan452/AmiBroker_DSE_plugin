#if !defined(AFX_CONFIGDLG_H__8EF41842_645F_466E_A069_C6A4AC4B8843__INCLUDED_)
#define AFX_CONFIGDLG_H__8EF41842_645F_466E_A069_C6A4AC4B8843__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfigDlg.h : header file
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
#include "Global.h"
#include "Plugin.h"

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog

class CConfigDlg : public CDialog
{
// Construction
public:
	void FillCombo( CStringList &oList, CComboBox &oCtrl );
	void FillTableList( void );
	CConfigDlg(CWnd* pParent = NULL);   // standard constructor

	struct InfoSite *m_pSite;

// Dialog Data
	//{{AFX_DATA(CConfigDlg)
	enum { IDD = IDD_CONFIG_DIALOG };
	CEdit	m_oDSNEdit;
	CComboBox	m_oVolumeCombo;
	CComboBox	m_oTableNameCombo;
	CComboBox	m_oSymbolCombo;
	CEdit	m_oSQLSymbolList;
	CEdit	m_oSQLQuotations;
	CButton	m_oSQLCustomQueryCheck;
	CComboBox	m_oOpenIntCombo;
	CComboBox	m_oOpenCombo;
	CComboBox	m_oLowCombo;
	CComboBox	m_oHighCombo;
	CComboBox	m_oDateCombo;
	CComboBox	m_oCloseCombo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnConnectButton();
	afx_msg void OnKillfocusTableNameCombo();
	virtual void OnOK();
	afx_msg void OnRetrieveButton();
	afx_msg void OnSqlCustomQueriesCheck();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGDLG_H__8EF41842_645F_466E_A069_C6A4AC4B8843__INCLUDED_)
