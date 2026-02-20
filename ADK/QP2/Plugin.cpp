////////////////////////////////////////////////////
// Plugin.cpp
// Standard implementation file for all AmiBroker plug-ins
//
// Copyright (C)2001 Tomasz Janeczko, amibroker.com
// All rights reserved.
//
// Last modified: 2001-11-30 TJ
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
#include "QP2.h"
#include "Plugin.h"
#include "ConfigDlg.h"

#include "usrdll22.h"
#pragma comment( lib, "qpdll.lib" )



// These are the only two lines you need to change
#define PLUGIN_NAME "Quotes Plus data Plug-in"
#define VENDOR_NAME "Amibroker.com"
#define PLUGIN_VERSION 20001

#define THIS_PLUGIN_TYPE PLUGIN_TYPE_DATA

////////////////////////////////////////
// Data section
////////////////////////////////////////
static struct PluginInfo oPluginInfo =
{
		sizeof( struct PluginInfo ),
		THIS_PLUGIN_TYPE,		
		PLUGIN_VERSION,
		PIDCODE( 'Q', 'P', '2', ' '),
		PLUGIN_NAME,
		VENDOR_NAME,
		13073095,
		381000
};

///////////////////////////////////////////////////////////
// Basic plug-in interface functions exported by DLL
///////////////////////////////////////////////////////////

PLUGINAPI int GetPluginInfo( struct PluginInfo *pInfo ) 
{ 
	*pInfo = oPluginInfo;

	return TRUE;
}



PLUGINAPI int Init(void) 
{ 
	HKEY hKey;					//Handle to Quotes Plus Setup Key
	DWORD dwType, dwSize;		//Registry value type and size
	char cpDataDir[512];		//Quotes Plus Data Directory


	//----------Get the data directory from the registry------
	if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Quotes Plus\\Setup",
					  0, KEY_READ, &hKey ) != ERROR_SUCCESS )
	{
		MessageBox( NULL, "Unable To Open Quotes Plus Setup Key!", NULL,
					MB_OK | MB_ICONSTOP | MB_TASKMODAL );
		return 0;	//Exit Application
	}
	if( RegQueryValueEx( hKey, "DataDir", 0, &dwType, 0, &dwSize )
						 != ERROR_SUCCESS )
	{
		RegCloseKey( hKey );
		return 0;
	}
	if( RegQueryValueEx( hKey, "DataDir", 0, &dwType, (LPBYTE)cpDataDir,
						 &dwSize ) != ERROR_SUCCESS )
	{
		RegCloseKey( hKey );
		return 0;
	}
	
	RegCloseKey( hKey );
	
	//Initialize Quotes Plus Dll
	if( R2_InitReadStock( cpDataDir, TRUE, NULL, FALSE ) != 0 )
		return 0;

	//------------Open Master Files and Zacks Data Files----------
	if( R2_Open_Files( MASTER_FILE, cpDataDir ) != 0 )
	{
		R2_Done();
		return 0;
	}

	if( R2_Open_Files( FUND_MASTER , cpDataDir ) != 0 )
	{
		R2_Done();
		return 0;
	}

	if( R2_Open_Files( FUTURE_MASTER , cpDataDir ) != 0 )
	{
		R2_Done();
		return 0;
	}

	if( R2_Open_Files( ZACKS_DATA, cpDataDir ) != 0 )
	{
		R2_Done();
		return 0;
	}

	if( R2_Open_Files( EPS_RANK_TABLE, cpDataDir ) != 0 )
	{
		R2_Done();
		return 0;
	}

	//------------Open Master Files and Zacks Data Files----------
	if( R2_Open_Files( ALL_DATA_FILES, cpDataDir ) != 0 )
	{
		R2_Done();
		return 0;
	}

	if( R2_Open_Files( IRL_DATA, cpDataDir ) != 0 )
	{
		R2_Done();
		return 0;
	}

	if( R2_Open_Files( IDXCOMP_TABLE, cpDataDir ) != 0 )
	{
		R2_Done();
		return 0;
	}


	return 1; 	 // default implementation does nothing

};	 

PLUGINAPI int Release(void) 
{ 
	R2_Close_Files( IDXCOMP_TABLE );

	R2_Close_Files( IRL_DATA );

	R2_Close_Files( ALL_DATA_FILES );

	R2_Close_Files( EPS_RANK_TABLE );

	R2_Close_Files( ZACKS_DATA );

	R2_Close_Files( FUTURE_MASTER );

	R2_Close_Files( FUND_MASTER );

	R2_Close_Files( MASTER_FILE );
	
	R2_Done();	

	return 1; 	  // default implementation does nothing
}; 

PLUGINAPI int GetQuotes( LPCTSTR pszTicker, int nPeriodicity, int nLastValid, int nSize, struct QuotationFormat4 *pQuotes  )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	if( nPeriodicity != PERIODICITY_EOD ) return 0;

	int iItem = 0;

	Master_Rec MasterRec;				//Quotes Plus Master Record

	Stock_Record *aRec = new Stock_Record[ nSize ];

	int nQty = 0;

	try
	{
		BOOL bRawData = AfxGetApp()->GetProfileInt( "QP2", "RawData", FALSE );

		if( nLastValid == nSize - 1 )
		{
			//
			// Simple cache validity check
			//

			nQty = R2_QP_LoadSymbol( 
							   pszTicker,
							   aRec/*Pointer to Price data record(s)*/,
							   1/*Number of records/days to get(NOTE:lpStockRec must 
						 point to a valid memory block that can store 
						 max_rec * sizeof( Stock_Record) bytes of data*/,
							bRawData/*Load Unadjusted data*/, 
							TRUE/*Do not	return days that are holidays*/,
							&MasterRec/*Pointer to Master_Rec*/
			);

			if( nQty == 1 && 
				(ULONG)( aRec[ 0 ].YY - 1900 ) == pQuotes[ nLastValid ].PackDate.Year &&
				(ULONG)( aRec[ 0 ].MM ) == pQuotes[ nLastValid ].PackDate.Month &&
				(ULONG)( aRec[ 0 ].DD ) == pQuotes[ nLastValid ].PackDate.Day )
			{

				// update the most recent bar
				pQuotes[ nLastValid ].Open			= aRec[ 0 ].open;
				pQuotes[ nLastValid ].High			= aRec[ 0 ].hi;
				pQuotes[ nLastValid ].Low			= aRec[ 0 ].lo;
				pQuotes[ nLastValid ].Price			= aRec[ 0 ].cl;
				pQuotes[ nLastValid ].OpenInterest	= (int) aRec[ 0 ].oi;
				pQuotes[ nLastValid ].Volume		= aRec[ 0 ].vol;


				delete[] aRec;
				return nSize;		
			}

		}


		nQty = R2_QP_LoadSymbol( 
		                   pszTicker,
						   aRec/*Pointer to Price data record(s)*/,
						   nSize/*Number of records/days to get(NOTE:lpStockRec must 
					 point to a valid memory block that can store 
					 max_rec * sizeof( Stock_Record) bytes of data*/,
						bRawData/*Load Unadjusted data*/, 
						TRUE/*Do not	return days that are holidays*/,
					    &MasterRec/*Pointer to Master_Rec*/
		);

		if( nQty == 0 )
		{
			nQty = R2_QP_LoadFundSymbol( 
							   pszTicker,
							   aRec/*Pointer to Price data record(s)*/,
							   nSize/*Number of records/days to get(NOTE:lpStockRec must 
						 point to a valid memory block that can store 
						 max_rec * sizeof( Stock_Record) bytes of data*/,
							bRawData/*Load Unadjusted data*/, 
							TRUE/*Do not	return days that are holidays*/
			);
		}


		for ( int iCnt = nSize - nQty; iCnt < nSize ; iCnt++ )
		{
			pQuotes[ iItem ].Date = (ULONG) -1L; // EOD
			pQuotes[ iItem ].PackDate.Year	= aRec[ iCnt ].YY - 1900; 
			pQuotes[ iItem ].PackDate.Month	= aRec[ iCnt ].MM;;
			pQuotes[ iItem ].PackDate.Day	= aRec[ iCnt ].DD;
			pQuotes[ iItem ].Open		= aRec[ iCnt ].open;
			pQuotes[ iItem ].High		= aRec[ iCnt ].hi;
			pQuotes[ iItem ].Low			= aRec[ iCnt ].lo;
			pQuotes[ iItem ].Price		= aRec[ iCnt ].cl;
			pQuotes[ iItem ].OpenInterest= (int) aRec[ iCnt ].oi;
			pQuotes[ iItem ].Volume		= aRec[ iCnt ].vol;
		
			iItem++;
		}
	}
	catch( ... )
	{
	}

	delete[] aRec;


	return iItem;
}

PLUGINAPI int GetQuotesEx( LPCTSTR pszTicker, int nPeriodicity, int nLastValid, int nSize, struct Quotation *pQuotes, GQEContext *pContext  )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	if( nPeriodicity != PERIODICITY_EOD ) return 0;

	int iItem = 0;

	Master_Rec MasterRec;				//Quotes Plus Master Record

	Stock_Record *aRec = new Stock_Record[ nSize ];

	int nQty = 0;

	try
	{
		BOOL bRawData = AfxGetApp()->GetProfileInt( "QP2", "RawData", FALSE );

		if( nLastValid == nSize - 1 )
		{
			//
			// Simple cache validity check
			//

			nQty = R2_QP_LoadSymbol( 
							   pszTicker,
							   aRec/*Pointer to Price data record(s)*/,
							   1/*Number of records/days to get(NOTE:lpStockRec must 
						 point to a valid memory block that can store 
						 max_rec * sizeof( Stock_Record) bytes of data*/,
							bRawData/*Load Unadjusted data*/, 
							TRUE/*Do not	return days that are holidays*/,
							&MasterRec/*Pointer to Master_Rec*/
			);

			if( nQty == 1 && 
				(ULONG)( aRec[ 0 ].YY ) == pQuotes[ nLastValid ].DateTime.PackDate.Year &&
				(ULONG)( aRec[ 0 ].MM ) == pQuotes[ nLastValid ].DateTime.PackDate.Month &&
				(ULONG)( aRec[ 0 ].DD ) == pQuotes[ nLastValid ].DateTime.PackDate.Day )
			{

				// update the most recent bar
				pQuotes[ nLastValid ].Open			= aRec[ 0 ].open;
				pQuotes[ nLastValid ].High			= aRec[ 0 ].hi;
				pQuotes[ nLastValid ].Low			= aRec[ 0 ].lo;
				pQuotes[ nLastValid ].Price			= aRec[ 0 ].cl;
				pQuotes[ nLastValid ].OpenInterest	= aRec[ 0 ].oi;
				pQuotes[ nLastValid ].Volume		= (float) aRec[ 0 ].vol;


				delete[] aRec;
				return nSize;		
			}

		}


		nQty = R2_QP_LoadSymbol( 
		                   pszTicker,
						   aRec/*Pointer to Price data record(s)*/,
						   nSize/*Number of records/days to get(NOTE:lpStockRec must 
					 point to a valid memory block that can store 
					 max_rec * sizeof( Stock_Record) bytes of data*/,
						bRawData/*Load Unadjusted data*/, 
						TRUE/*Do not	return days that are holidays*/,
					    &MasterRec/*Pointer to Master_Rec*/
		);

		if( nQty == 0 )
		{
			nQty = R2_QP_LoadFundSymbol( 
							   pszTicker,
							   aRec/*Pointer to Price data record(s)*/,
							   nSize/*Number of records/days to get(NOTE:lpStockRec must 
						 point to a valid memory block that can store 
						 max_rec * sizeof( Stock_Record) bytes of data*/,
							bRawData/*Load Unadjusted data*/, 
							TRUE/*Do not	return days that are holidays*/
			);
		}


		for ( int iCnt = nSize - nQty; iCnt < nSize ; iCnt++ )
		{
			pQuotes[ iItem ].DateTime.Date = DAILY_MASK; // EOD
			pQuotes[ iItem ].DateTime.PackDate.Year	= aRec[ iCnt ].YY; 
			pQuotes[ iItem ].DateTime.PackDate.Month	= aRec[ iCnt ].MM;;
			pQuotes[ iItem ].DateTime.PackDate.Day	= aRec[ iCnt ].DD;
			pQuotes[ iItem ].Open		= aRec[ iCnt ].open;
			pQuotes[ iItem ].High		= aRec[ iCnt ].hi;
			pQuotes[ iItem ].Low			= aRec[ iCnt ].lo;
			pQuotes[ iItem ].Price		= aRec[ iCnt ].cl;
			pQuotes[ iItem ].OpenInterest= aRec[ iCnt ].oi;
			pQuotes[ iItem ].Volume		= (float) aRec[ iCnt ].vol;
		
			iItem++;
		}
	}
	catch( ... )
	{
	}

	delete[] aRec;


	return iItem;
}


PLUGINAPI int Configure( LPCTSTR pszPath, struct InfoSite *pSite )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	CConfigDlg oDlg;

	oDlg.m_pSite = pSite;

	oDlg.DoModal();


	return 1;
}

PLUGINAPI AmiVar GetExtraData( LPCTSTR pszTicker, LPCTSTR pszName, int nArraySize, int nPeriodicity, void* (*pfAlloc)(unsigned int nSize) )
{
	AmiVar var;
	Master_Rec Rec;
	QP_ZACKS_DATA ZacksData;

	int rc;
	
	rc = R2_QP_ReadMaster( QP_EQUAL , QP_SYMBOL , pszTicker , &Rec ); // Start at the first stock

	if( rc != 0 )
	{
		rc = R2_QP_ReadFundMaster( QP_EQUAL , QP_SYMBOL , pszTicker , &Rec );
	}

	var.type = VAR_FLOAT;
	var.val = 0;

	BOOL bFound = FALSE;

	BOOL bEmptyText = FALSE;

	if( rc == 0	)
	{
		bFound = TRUE;

		if( !stricmp( pszName, "AnnDividend" ) )
		{
			var.val = Rec.IAD;
		}
		else
		if( !stricmp( pszName, "shares" ) )
		{
			var.val = (float) Rec.shares;
		}
		else
		if( !stricmp( pszName, "LastMainDate" ) )
		{
			var.val = (float) Rec.last_main_date;
		}
		else
		if( !stricmp( pszName, "hasoptions" ) )
		{
			var.val = (float) Rec.margin_flag == 79;
		}
		else
		if( !stricmp( pszName, "IssueType" ) )
		{
			var.type = VAR_STRING;
			var.string = (char *)pfAlloc( 2 );
			var.string[ 0 ] = Rec.issue_type;
			var.string[ 1 ] = '\0';
		}
		else
		if( !stricmp( pszName, "IssueStatus" ) )
		{
			var.type = VAR_STRING;
			var.string = (char *)pfAlloc( 2 );
			var.string[ 0 ] = Rec.issue_status;
			var.string[ 1 ] = '\0';
		}
		else
		if( !stricmp( pszName, "LastMainDateStr" ) )
		{
			var.type = VAR_STRING;
			var.string = (char *)pfAlloc( 20 );
			sprintf( var.string, "%d", Rec.last_main_date );
		}
		else
		if( !stricmp( pszName, "Exchange" ) )
		{
			var.type = VAR_STRING;
			var.string = (char *)pfAlloc( 3 );
			var.string[ 0 ] = Rec.exchange[ 0 ];
			var.string[ 1 ] = Rec.exchange[ 1 ];
			var.string[ 2 ] = '\0';
		}
		else
		if( !stricmp( pszName, "ExchangeSub" ) )
		{
			var.type = VAR_STRING;
			var.string = (char *)pfAlloc( 2 );
			var.string[ 0 ] = Rec.exchange_sub;
			var.string[ 1 ] = '\0';
		}
		else
		if( !stricmp( pszName, "Flags" ) )
		{
			var.type = VAR_STRING;
			var.string = (char *)pfAlloc( 2 );
			var.string[ 0 ] = Rec.flags;
			var.string[ 1 ] = '\0';
		}
		else
		if( !stricmp( pszName, "MarginFlag" ) )
		{
			var.type = VAR_STRING;
			var.string = (char *)pfAlloc( 2 );
			var.string[ 0 ] = Rec.margin_flag;
			var.string[ 1 ] = '\0';
		}
		else
		if( !stricmp( pszName, "CUSIP" ) )
		{
			var.type = VAR_STRING;
			var.string = (char *)pfAlloc( 9 );
			strncpy( var.string, Rec.CUSIP, 8 );
			var.string[ 8 ] = '\0';
		}
		else
		if( !stricmp( pszName, "SIC" ) )
		{
			var.type = VAR_STRING;
			var.string = (char *)pfAlloc( 5 );
			strncpy( var.string, Rec.SIC, 4 );
			var.string[ 4 ] = '\0';
		}
		else
		if( !stricmp( pszName, "QRS" ) )
		{
			var.type = VAR_ARRAY;
			var.array = (float *) pfAlloc( sizeof( float ) * nArraySize );

			Master_Rec MasterRec;				//Quotes Plus Master Record

			Stock_Record *aRec = new Stock_Record[ nArraySize ];

			int nQty = R2_QP_LoadSymbol( 
							   pszTicker,
							   aRec/*Pointer to Price data record(s)*/,
							   nArraySize/*Number of records/days to get(NOTE:lpStockRec must 
						 point to a valid memory block that can store 
						 max_rec * sizeof( Stock_Record) bytes of data*/,
							FALSE/*Load Unadjusted data*/, 
							TRUE/*Do not	return days that are holidays*/,
							&MasterRec/*Pointer to Master_Rec*/
			);

			if( nQty == 0 )
			{
				nQty = R2_QP_LoadFundSymbol( 
								   pszTicker,
								   aRec/*Pointer to Price data record(s)*/,
								   nArraySize/*Number of records/days to get(NOTE:lpStockRec must 
							 point to a valid memory block that can store 
							 max_rec * sizeof( Stock_Record) bytes of data*/,
								FALSE/*Load Unadjusted data*/, 
								TRUE/*Do not	return days that are holidays*/
				);
			}


			for ( int iCnt = 0; iCnt < nArraySize ; iCnt++ )
			{
				if( iCnt < nArraySize - nQty )
				{
					var.array[ iCnt ] = EMPTY_VAL;
				}
				else
				{
					var.array[ iCnt ] = aRec[ iCnt ].RS;
				}
			}

			delete[] aRec;
		}
		else
		if( !stricmp( pszName, "EPSRank" ) )
		{
			EPSRANK qpRankTable[512];

	   		int nEPSQty = R2_QP_ReadEpsRank( pszTicker, qpRankTable, 512 );

			if( nEPSQty > 0 )
			{
				var.type = VAR_ARRAY;
				var.array = (float *) pfAlloc( sizeof( float ) * nArraySize );


				Master_Rec MasterRec;				//Quotes Plus Master Record

				Stock_Record *aRec = new Stock_Record[ nArraySize ];

				int nQty = R2_QP_LoadSymbol( 
								   pszTicker,
								   aRec/*Pointer to Price data record(s)*/,
								   nArraySize/*Number of records/days to get(NOTE:lpStockRec must 
							 point to a valid memory block that can store 
							 max_rec * sizeof( Stock_Record) bytes of data*/,
								FALSE/*Load Unadjusted data*/, 
								TRUE/*Do not	return days that are holidays*/,
								&MasterRec/*Pointer to Master_Rec*/
				);

				int iEPSCnt = nEPSQty - 1;

				for ( int iCnt = 0; iCnt < nArraySize && iEPSCnt >= 0; iCnt++ )
				{
					if( iCnt < nArraySize - nQty )
					{
						var.array[ iCnt ] = EMPTY_VAL;
					}
					else
					{
					   int nBarDate = aRec[ iCnt ].YY * 10000 + aRec[ iCnt ].MM * 100 + aRec[ iCnt ].DD;

					   // skip top old entries
					   while( iEPSCnt > 0 && 
							  nBarDate >= qpRankTable[ iEPSCnt ].lDate  &&
							  nBarDate >= qpRankTable[ iEPSCnt - 1 ].lDate )	iEPSCnt--;

					   if( nBarDate < qpRankTable[ iEPSCnt ].lDate )
					   {
						   var.array[ iCnt ] = EMPTY_VAL;
					   }
					   else
					   if( nBarDate >= qpRankTable[ iEPSCnt ].lDate &&
						   ( iEPSCnt == 0 || nBarDate < qpRankTable[ iEPSCnt - 1 ].lDate ) )
					   {
						   var.array[ iCnt ] = qpRankTable[ iEPSCnt ].ucRank;
					   }
					   else
					   {
						   ASSERT(0); // PROBLEM
					   }
						
					}
				}

				delete[] aRec;

			}
		}
		else
		if( !stricmp( pszName, "Sales" ) ||
			!stricmp( pszName, "EPS" ) )
		{
			BOOL bGetEPS = !stricmp( pszName, "EPS" );

			QP_ZACKS_EPS qpZacksEPSTable[20];

	   		BOOL bOK = ! R2_QP_ReadZacksEPS( pszTicker, qpZacksEPSTable );

			int nEPSQty = 0;

			for( nEPSQty = 0; bOK && nEPSQty < 20; nEPSQty++ ) 
			{
				if( qpZacksEPSTable[ nEPSQty ].FiscQuart == 0 ) break;	
			}
				

			if( bOK )
			{
				var.type = VAR_ARRAY;
				var.array = (float *) pfAlloc( sizeof( float ) * nArraySize );


				Master_Rec MasterRec;				//Quotes Plus Master Record

				Stock_Record *aRec = new Stock_Record[ nArraySize ];

				int nQty = R2_QP_LoadSymbol( 
								   pszTicker,
								   aRec/*Pointer to Price data record(s)*/,
								   nArraySize/*Number of records/days to get(NOTE:lpStockRec must 
							 point to a valid memory block that can store 
							 max_rec * sizeof( Stock_Record) bytes of data*/,
								FALSE/*Load Unadjusted data*/, 
								TRUE/*Do not	return days that are holidays*/,
								&MasterRec/*Pointer to Master_Rec*/
				);

				int iEPSCnt = nEPSQty - 1;

				for ( int iCnt = 0; iCnt < nArraySize && iEPSCnt >= 0; iCnt++ )
				{
					if( iCnt < nArraySize - nQty )
					{
						var.array[ iCnt ] = EMPTY_VAL;
					}
					else
					{
					   int nBarDate = aRec[ iCnt ].YY * 100 + aRec[ iCnt ].MM;

					   // skip top old entries
					   while( iEPSCnt > 0 && 
							  nBarDate > qpZacksEPSTable[ iEPSCnt ].FiscQuart  &&
							  nBarDate > qpZacksEPSTable[ iEPSCnt - 1 ].FiscQuart ) iEPSCnt--;

					   if( nBarDate < qpZacksEPSTable[ iEPSCnt ].FiscQuart )
					   {
						   var.array[ iCnt ] = EMPTY_VAL;
					   }
					   else
					   if( nBarDate > qpZacksEPSTable[ iEPSCnt ].FiscQuart &&
						   ( iEPSCnt == 0 || nBarDate <= qpZacksEPSTable[ iEPSCnt - 1 ].FiscQuart ) )
					   {
						   var.array[ iCnt ] = bGetEPS ? qpZacksEPSTable[ iEPSCnt ].EPS : qpZacksEPSTable[ iEPSCnt ].Sales;
					   }
					   else
					   {
						   var.array[ iCnt ] = EMPTY_VAL;
					   }
						
					}
				}

				delete[] aRec;

			}
			else
			{
				var.type = VAR_FLOAT;
				var.val = EMPTY_VAL;
			}

		}
		else
		{
			bFound = FALSE;
		}
	}
	else
	{
		if( !stricmp( pszName, "IssueStatus" ) ||
		    !stricmp( pszName, "IssueType" ) ||
		    !stricmp( pszName, "Exchange" ) ||
		    !stricmp( pszName, "ExchangeSub" ) ||
		    !stricmp( pszName, "Flags" ) ||
		    !stricmp( pszName, "MarginFlag" ) ||
		    !stricmp( pszName, "CUSIP" ) ||
		    !stricmp( pszName, "LastMainDateStr" ) ||
		    !stricmp( pszName, "SIC" ) 
			) bEmptyText = TRUE;
	}

	if( !bFound )
	{
		rc += R2_QP_ReadZacksData( pszTicker, &ZacksData );
	}

	if( rc == 0	&& !bFound)
	{
		bFound = TRUE;

		if( !stricmp( pszName, "sharesfloat" ) )
		{
			var.val = ZacksData.sharesFloat;
		}
		else
		if( !stricmp( pszName, "sharesout" ) )
		{
			var.val = ZacksData.SharesOut;
		}
		else
		if( !stricmp( pszName, "sharesshort" ) )
		{
			var.val = (float) ZacksData.SharesShort;
		}
		else
		if( !stricmp( pszName, "ttmsales" ) )
		{
			var.val = ZacksData.TTMSales;
		}
		else
		if( !stricmp( pszName, "beta" ) )
		{
			var.val = ZacksData.Beta;
		}
		else
		if( !stricmp( pszName, "ttmeps" ) )
		{
			var.val = ZacksData.TTMEPS;
		}
		else
		if( !stricmp( pszName, "HiPERange" ) )
		{
			var.val = ZacksData.HiPERange;
		}
		else
		if( !stricmp( pszName, "LoPERange" ) )
		{
			var.val = ZacksData.LoPERange;
		}
		else
		if( !stricmp( pszName, "PEG" ) )
		{
			var.val = ZacksData.PEG;
		}
		else
		if( !stricmp( pszName, "instholds" ) )
		{
			var.val = ZacksData.InstHolds;
		}
		else
		if( !stricmp( pszName, "LTDebtToEq" ) )
		{
			var.val = ZacksData.LTDebtToEq;
		}
		else
		if( !stricmp( pszName, "CashFlowPerShare" ) )
		{
			var.val = ZacksData.CashFlowPerShare;
		}
		else
		if( !stricmp( pszName, "ROE" ) )
		{
			var.val = ZacksData.ROE;
		}
		else
		if( !stricmp( pszName, "ttmsales" ) )
		{
			var.val = ZacksData.TTMSales;
		}
		else
		if( !stricmp( pszName, "Yr1EPSGrowth" ) )
		{
			var.val = ZacksData.Yr1EPSGrowth;
		}
		else
		if( !stricmp( pszName, "Yr5EPSGrowth" ) )
		{
			var.val = ZacksData.Yr5EPSGrowth;
		}
		else
		if( !stricmp( pszName, "Yr1ProjEPSGrowth" ) )
		{
			var.val = ZacksData.Yr1ProjEPSGr;
		}
		else
		if( !stricmp( pszName, "Yr2ProjEPSGrowth" ) )
		{
			var.val = ZacksData.Yr2ProjEPSGr;
		}
		else
		if( !stricmp( pszName, "Yr3to5ProjEPSGrowth" ) )
		{
			var.val = ZacksData.Yr3to5ProjEPSGr;
		}
		else
		if( !stricmp( pszName, "BookValuePerShare" ) )
		{
			var.val = ZacksData.BookValuePerShare;
		}
		else
		if( !stricmp( pszName, "Briefing" ) )
		{
			var.type = VAR_STRING;
			var.string = (char *)pfAlloc( sizeof( ZacksData.Briefing ) + 1 );
			var.string[ sizeof( ZacksData.Briefing ) ] = '\0';
			strncpy( var.string, ZacksData.Briefing, sizeof( ZacksData.Briefing ) );
		}
		else
		{
			var.type = VAR_NONE;
		}

/*
	char  source;		//The source of the data - Z for Zacks or M for Market Guide  added 1/12/2000
	char  currency[18];  //The currency that the numbers are reported in
*/
	}
	else
	{
		if( !bFound	&& !stricmp( pszName, "Briefing" ) ) bEmptyText = TRUE;
	}

	if( bEmptyText )
	{
		var.type = VAR_STRING;
		var.string = (char *)pfAlloc( 1 );
		var.string[ 0 ] = '\0';
	}

	return var;
}
