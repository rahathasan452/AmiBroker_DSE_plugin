// ConfigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "QT.h"
#include "Plugin.h"
#include "ConfigDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern int		g_nPortNumber;
extern int		g_nRefreshInterval;
extern BOOL		g_bAutoAddSymbols;
extern int		g_nSymbolLimit;
extern BOOL		g_bOptimizedIntraday;
extern int		g_nTimeShift;
extern CString  g_oServer;




extern CString GetAvailableSymbols( void );

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog

CConfigDlg::CConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfigDlg)
	//}}AFX_DATA_INIT
}


void CConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigDlg)
	//}}AFX_DATA_MAP
	DDX_Text( pDX, IDC_SERVER_EDIT, g_oServer );

	DDX_Check( pDX, IDC_AUTOSYMBOLS_CHECK, g_bAutoAddSymbols );
	DDX_Text( pDX, IDC_PORT_EDIT, g_nPortNumber );
	DDV_MinMaxInt( pDX, g_nPortNumber, 1, 65535 );

	DDX_Text( pDX, IDC_INTERVAL_EDIT, g_nRefreshInterval );
	DDV_MinMaxInt( pDX, g_nRefreshInterval, 1, 60 );

	DDX_Text( pDX, IDC_MAXSYMBOL_EDIT, g_nSymbolLimit );
	DDV_MinMaxInt( pDX, g_nSymbolLimit, 10, 500 );

	DDX_Check( pDX, IDC_OPTIMIZED_INTRADAY_CHECK, g_bOptimizedIntraday );

	DDX_Text( pDX, IDC_TIMESHIFT_EDIT, g_nTimeShift );
	DDV_MinMaxInt( pDX, g_nTimeShift, -48, 48 );
}


BEGIN_MESSAGE_MAP(CConfigDlg, CDialog)
	//{{AFX_MSG_MAP(CConfigDlg)
	ON_BN_CLICKED(IDC_RETRIEVE_BUTTON, OnRetrieveButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg message handlers



BOOL CConfigDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigDlg::OnOK() 
{
	CDialog::OnOK();

	AfxGetApp()->WriteProfileInt( "QuoteTracker", "TimeShift", g_nTimeShift );
	AfxGetApp()->WriteProfileInt( "QuoteTracker", "Port", g_nPortNumber );
	AfxGetApp()->WriteProfileString( "QuoteTracker", "Server", g_oServer );
	AfxGetApp()->WriteProfileInt( "QuoteTracker", "RefreshInterval", g_nRefreshInterval );
	AfxGetApp()->WriteProfileInt( "QuoteTracker", "AutoAddSymbols", g_bAutoAddSymbols );
	AfxGetApp()->WriteProfileInt( "QuoteTracker", "SymbolLimit", g_nSymbolLimit );
	AfxGetApp()->WriteProfileInt( "QuoteTracker", "OptimizedIntraday", g_bOptimizedIntraday );
}

void CConfigDlg::OnRetrieveButton() 
{
	CString oSymbolList = GetAvailableSymbols();

	CString oSymbol;

	for( int iCnt = 0; AfxExtractSubString( oSymbol, oSymbolList, iCnt, ',' ); iCnt++ )
	{
		if( oSymbol != "MEDVED" ) m_pSite->AddStock( oSymbol );
	}

	oSymbol.Format("Retrieved %d symbols", iCnt );

	SetDlgItemText( IDC_STATUS_STATIC, oSymbol );
}
