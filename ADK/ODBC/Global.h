// Global.h: interface for the CGlobal class.
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

#if !defined(AFX_GLOBAL_H__13CDDD97_85E9_4AD4_B7C3_78C5B20342F3__INCLUDED_)
#define AFX_GLOBAL_H__13CDDD97_85E9_4AD4_B7C3_78C5B20342F3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CGlobal  
{
public:
	CGlobal();
	virtual ~CGlobal();

public:
	void UpdateServerType();
	CString Wrap( const CString &src );
	void StoreErrorMessage( CDBException * e );
	CDatabase * GetAflDatabase();
	CString m_oAflSymbolField;
	CString m_oAflDateField;
	BOOL ConnectAFL( LPCTSTR pszConnectString );
	void GetFieldArray( CStringArray &oArr );
	CString GetQuoteSQL( LPCTSTR pszSymbol, int nSize );
	CDatabase * GetQuoteDatabase();
	void Disconnect();
	void Connect();
	void Save();
	void Load();
	CString	m_oClose;
	CString	m_oDate;
	CString	m_oDSN;
	CString	m_oHigh;
	CString	m_oLow;
	CString	m_oOpen;
	CString	m_oOpenInt;
	BOOL	m_bSQLCustomQuery;
	CString	m_oSQLQuotations;
	CString	m_oSQLSymbolList;
	CString	m_oSymbol;
	CString	m_oTableName;
	CString	m_oVolume;

	CString m_oAflDSN;

	CString	m_oWrapMarks[ 2 ];

	CDatabase *m_poQuoteDB;
	CDatabase *m_poAflDB;

	BOOL	m_bAutoRefresh;

	HWND	m_hAmiBrokerWnd;

	CString m_oLastErrorMsg;

	BOOL	m_bDisplayErrors;

	int		m_iServerType;

};


extern CGlobal g_oData;

#endif // !defined(AFX_GLOBAL_H__13CDDD97_85E9_4AD4_B7C3_78C5B20342F3__INCLUDED_)
