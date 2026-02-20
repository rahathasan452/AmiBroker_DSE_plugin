// ConfigDlg.cpp : implementation file
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
#include "ConfigDlg.h"
#include "SRecordset.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
	DDX_Control(pDX, IDC_DSN_EDIT, m_oDSNEdit);
	DDX_Control(pDX, IDC_VOLUME_COMBO, m_oVolumeCombo);
	DDX_Control(pDX, IDC_TABLE_NAME_COMBO, m_oTableNameCombo);
	DDX_Control(pDX, IDC_SYMBOL_COMBO, m_oSymbolCombo);
	DDX_Control(pDX, IDC_SQL_SYMBOL_LIST_EDIT, m_oSQLSymbolList);
	DDX_Control(pDX, IDC_SQL_QUOTATIONS_EDIT, m_oSQLQuotations);
	DDX_Control(pDX, IDC_SQL_CUSTOM_QUERIES_CHECK, m_oSQLCustomQueryCheck);
	DDX_Control(pDX, IDC_OPEN_INT_COMBO, m_oOpenIntCombo);
	DDX_Control(pDX, IDC_OPEN_COMBO, m_oOpenCombo);
	DDX_Control(pDX, IDC_LOW_COMBO, m_oLowCombo);
	DDX_Control(pDX, IDC_HIGH_COMBO, m_oHighCombo);
	DDX_Control(pDX, IDC_DATE_COMBO, m_oDateCombo);
	DDX_Control(pDX, IDC_CLOSE_COMBO, m_oCloseCombo);
	//}}AFX_DATA_MAP
	DDX_CBString(pDX, IDC_CLOSE_COMBO, g_oData.m_oClose);
	DDX_CBString(pDX, IDC_DATE_COMBO, g_oData.m_oDate);
	DDX_Text(pDX, IDC_DSN_EDIT, g_oData.m_oDSN);
	DDX_CBString(pDX, IDC_HIGH_COMBO, g_oData.m_oHigh);
	DDX_CBString(pDX, IDC_LOW_COMBO, g_oData.m_oLow);
	DDX_CBString(pDX, IDC_OPEN_COMBO, g_oData.m_oOpen);
	DDX_CBString(pDX, IDC_OPEN_INT_COMBO, g_oData.m_oOpenInt);
	DDX_Check(pDX, IDC_SQL_CUSTOM_QUERIES_CHECK, g_oData.m_bSQLCustomQuery);
	DDX_Text(pDX, IDC_SQL_QUOTATIONS_EDIT, g_oData.m_oSQLQuotations);
	DDX_Text(pDX, IDC_SQL_SYMBOL_LIST_EDIT, g_oData.m_oSQLSymbolList);
	DDX_CBString(pDX, IDC_SYMBOL_COMBO, g_oData.m_oSymbol);
	DDX_CBString(pDX, IDC_TABLE_NAME_COMBO, g_oData.m_oTableName);
	DDX_CBString(pDX, IDC_VOLUME_COMBO, g_oData.m_oVolume);
	DDX_Check(pDX, IDC_AUTO_REFRESH_CHECK, g_oData.m_bAutoRefresh);
	DDX_CBIndex(pDX, IDC_SERVER_TYPE_COMBO, g_oData.m_iServerType);
}


BEGIN_MESSAGE_MAP(CConfigDlg, CDialog)
	//{{AFX_MSG_MAP(CConfigDlg)
	ON_BN_CLICKED(IDC_CONNECT_BUTTON, OnConnectButton)
	ON_CBN_KILLFOCUS(IDC_TABLE_NAME_COMBO, OnKillfocusTableNameCombo)
	ON_BN_CLICKED(IDC_RETRIEVE_BUTTON, OnRetrieveButton)
	ON_BN_CLICKED(IDC_SQL_CUSTOM_QUERIES_CHECK, OnSqlCustomQueriesCheck)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg message handlers

BOOL CConfigDlg::OnInitDialog() 
{
	g_oData.Load();

	CDialog::OnInitDialog();

	OnSqlCustomQueriesCheck();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigDlg::OnConnectButton() 
{
	if( g_oData.m_poQuoteDB == NULL )
	{
		g_oData.m_poQuoteDB = new CDatabase;
	}
	else
	{
		g_oData.m_poQuoteDB->Close();
	}

	try
	{
		g_oData.m_poQuoteDB->OpenEx( NULL );	

		g_oData.m_oDSN = g_oData.m_poQuoteDB->GetConnect();
		m_oDSNEdit.SetWindowText( g_oData.m_oDSN );

		FillTableList();

	}
	catch( CDBException *e )
	{	 
		CString strFormatted;

		strFormatted.Format(_T("ODBC driver returned following exception:\n\n%d\n%s\n%s"), e->m_nRetCode, e->m_strError, e->m_strStateNativeOrigin);
		AfxMessageBox( strFormatted );

		e->Delete();

		delete g_oData.m_poQuoteDB;
		g_oData.m_poQuoteDB = NULL;
	}
}

void CConfigDlg::OnKillfocusTableNameCombo() 
{
	CString oTableName;
	m_oTableNameCombo.GetWindowText( oTableName );


	if( g_oData.m_poQuoteDB && g_oData.m_poQuoteDB->IsOpen() && ! oTableName.IsEmpty() )
	{
		CString oSQL;

		oTableName.Replace("{SYMBOL}", "AA" );
	
		oSQL.Format("SELECT * FROM %s WHERE 0=1", oTableName );

		CStringList oFieldList;

		try
		{
			CWaitCursor oWait;

			CSRecordset oSet( g_oData.m_poQuoteDB );

			if( oSet.Open( CRecordset::forwardOnly, oSQL ) )
			{
				int iQty = oSet.GetODBCFieldCount();
				
				for( int iField = 0; iField < iQty; iField++ )
				{
					CODBCFieldInfo oInfo;
					oSet.GetODBCFieldInfo( (short) iField, oInfo );

					oFieldList.AddTail( oInfo.m_strName	);
				}
			}

		}
		catch( CDBException *e )
		{	 
			CString strFormatted;

			strFormatted.Format(_T("ODBC driver returned following exception:\n\n%d\n%s\n%s"), e->m_nRetCode, e->m_strError, e->m_strStateNativeOrigin);
			AfxMessageBox( strFormatted );

			e->Delete();
		}
	

		if( ! oFieldList.IsEmpty() )
		{
			FillCombo( oFieldList, m_oOpenCombo );
			FillCombo( oFieldList, m_oHighCombo );
			FillCombo( oFieldList, m_oLowCombo );
			FillCombo( oFieldList, m_oCloseCombo );
			FillCombo( oFieldList, m_oVolumeCombo );
			FillCombo( oFieldList, m_oOpenIntCombo );
			FillCombo( oFieldList, m_oDateCombo );
			FillCombo( oFieldList, m_oSymbolCombo );
		}
	}
}

void CConfigDlg::FillTableList()
{
	if( g_oData.m_poQuoteDB && g_oData.m_poQuoteDB->IsOpen() )
	{
		HDBC  hdbc = g_oData.m_poQuoteDB->m_hdbc;
		HSTMT hstmt;
		UCHAR tabQualifier[255], tabOwner[255], tabName[255];
		UCHAR tabType[255], remarks[255];
		SDWORD lenTabQualifier, lenTabOwner, lenTableName;
		SDWORD lenTableType, lenRemarks;
		SDWORD retcode;

		retcode = SQLAllocHandle(SQL_HANDLE_STMT,hdbc,&hstmt);
		retcode = SQLTables(hstmt,
						   (UCHAR FAR *)NULL, 0,             /* tabQualifier  */
						   (UCHAR FAR *)NULL, 0,             /* tabOwners     */
						   (UCHAR FAR *)"%", SQL_NTS,   /* table name    */
						   (UCHAR FAR *)"TABLE", SQL_NTS);   /* table type    */
		/* Bind columns in result set to storage locations                    */
		retcode = SQLBindCol(hstmt, 1, SQL_C_CHAR, tabQualifier, 255,
							 &lenTabQualifier);
		retcode = SQLBindCol(hstmt, 2, SQL_C_CHAR, tabOwner, 255, &lenTabOwner);
		retcode = SQLBindCol(hstmt, 3, SQL_C_CHAR, tabName, 255, &lenTableName);
		retcode = SQLBindCol(hstmt, 4, SQL_C_CHAR, tabType, 255, &lenTableType);
		retcode = SQLBindCol(hstmt, 5, SQL_C_CHAR, remarks, 255, &lenRemarks);
		/* print out the records in the result set                            */

		m_oTableNameCombo.ResetContent();

		while ((retcode = SQLFetch(hstmt)) == SQL_SUCCESS)
		{                        
			   TRACE("column 1 : table qualifier = %s\n", tabQualifier);
			   TRACE("column 2 : table owner = %s\n", tabOwner);
			   TRACE("column 3 : table name = %s\n", tabName);
			   TRACE("column 4 : table type = %s\n", tabType);
			   TRACE("column 5 : remarks = %s\n", remarks);

				m_oTableNameCombo.AddString( (LPCTSTR) tabName );
		}

		SQLFreeHandle( SQL_HANDLE_STMT, hstmt );

	}

}

void CConfigDlg::FillCombo(CStringList &oList, CComboBox &oCtrl)
{
	POSITION hPos = oList.GetHeadPosition();

	CString oText;

	oCtrl.GetWindowText( oText );

	oCtrl.ResetContent();

	while( hPos )
	{
		oCtrl.AddString( oList.GetNext( hPos ) );
	}

	int iItem = oCtrl.FindStringExact( -1, oText );

	if( iItem != -1 ) oCtrl.SetCurSel( iItem );
}

void CConfigDlg::OnOK() 
{

	
	CDialog::OnOK();

	g_oData.Save();
}

void CConfigDlg::OnRetrieveButton() 
{
	CString oTableName, oSymbolField;
	m_oTableNameCombo.GetWindowText( oTableName );
	m_oSymbolCombo.GetWindowText( oSymbolField );

	if( g_oData.m_poQuoteDB && g_oData.m_poQuoteDB->IsOpen() && 
		! oTableName.IsEmpty() &&
		! oSymbolField.IsEmpty() )
	{
		CString oSQL;

		m_oSQLSymbolList.GetWindowText( oSQL );

		if( m_oSQLCustomQueryCheck.GetCheck() && ! oSQL.IsEmpty() )
		{
			//oSQL contains good SQL now
		}
		else
		{
			oSQL.Format("SELECT %s FROM %s WHERE 1=1 GROUP BY %s", 
				oSymbolField, oTableName, oSymbolField );
		}

		CStringList oFieldList;

		try
		{
			CWaitCursor oWait;

			CSRecordset oSet( g_oData.m_poQuoteDB );

			if( oSet.Open( CRecordset::forwardOnly, oSQL ) )
			{
				while( ! oSet.IsEOF() )
				{
					CString oSymbol;

					oSet.GetFieldValue( oSymbolField, oSymbol );

					m_pSite->AddStock( oSymbol );

					oSet.MoveNext();
				}
			}

		}
		catch( CDBException *e )
		{	 
			CString strFormatted;

			strFormatted.Format(_T("ODBC driver returned following exception:\n\n%d\n%s\n%s"), e->m_nRetCode, e->m_strError, e->m_strStateNativeOrigin);
			AfxMessageBox( strFormatted );

			e->Delete();
		}
	

	}
	else
	{
		AfxMessageBox("You need to specify database, table and symbol field first");
	}
	
}

void CConfigDlg::OnSqlCustomQueriesCheck() 
{
	BOOL bEnable = m_oSQLCustomQueryCheck.GetCheck();
	
	m_oSQLQuotations.EnableWindow( bEnable );
	m_oSQLSymbolList.EnableWindow( bEnable );
}
