#if !defined(AFX_CONFIGDLG_H__0D702E26_EA7B_4D02_91AD_600CA8E8B5D5__INCLUDED_)
#define AFX_CONFIGDLG_H__0D702E26_EA7B_4D02_91AD_600CA8E8B5D5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfigDlg.h : header file
//
#include "usrdll22.h"
/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog

class CConfigDlg : public CDialog
{
// Construction
public:
	BOOL HasQuotes( Master_Rec *pRec, BOOL bFund = FALSE );
	BOOL HasQuotes( LPCTSTR pszSymbol, BOOL bFund = FALSE );
	void SetupGroups( void );
	void RetrieveContractFutures();
	void RetrieveCommodityFutures(void);
	void RetrieveCashFutures();
	void RetrieveSectors();
	void RetrieveStocks();
	void RetrieveFunds();
	struct StockInfoFormat4 * AddStock( Master_Rec *pRec );
	void AddComponents( QP_IRL_REC *pIrlRec, InfoSite *pSite );
	struct InfoSite * m_pSite;
	CConfigDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfigDlg)
	enum { IDD = IDD_CONFIG_DIALOG };
	CStatic	m_oStatusStatic;
	BOOL	m_bCash;
	BOOL	m_bCommodity;
	BOOL	m_bFund;
	BOOL	m_bIndex;
	BOOL	m_bSector;
	BOOL	m_bStock;
	int		m_iContType;
	BOOL	m_bRawData;
	BOOL	m_bOnlyActivelyTraded;
	BOOL	m_bExcludeNoQuotes;
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
	afx_msg void OnRetrieveButton();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGDLG_H__0D702E26_EA7B_4D02_91AD_600CA8E8B5D5__INCLUDED_)
