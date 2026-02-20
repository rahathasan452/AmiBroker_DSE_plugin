////////////////////////////////////////////////////
// Plugin.cpp
// Standard implementation file for AmiBroker data plug-ins
//
///////////////////////////////////////////////////////////////////////
// Copyright (c) 2001-2009 AmiBroker.com. All rights reserved. 
//
// Users and possessors of this source code are hereby granted a nonexclusive, 
// royalty-free copyright license to use this code in individual and commercial software.
//
// AMIBROKER.COM MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE CODE FOR ANY PURPOSE. 
// IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND. 
// AMIBROKER.COM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOURCE CODE, 
// INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. 
// IN NO EVENT SHALL AMIBROKER.COM BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL, OR 
// CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, 
// WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, 
// ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE CODE.
// 
// Any use of this source code must include the above notice, 
// in the user documentation and internal comments to the code.
///////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Plugin.h"
#include "resource.h"
#include "ConfigDlg.h"


// These are the only two lines you need to change
#define PLUGIN_NAME "QuoteTracker data Plug-in"
#define VENDOR_NAME "Amibroker.com"
#define PLUGIN_VERSION 20001
#define PLUGIN_ID PIDCODE( 'Q', 'T', 'R', 'K')

// IMPORTANT: Define plugin type !!!
#define THIS_PLUGIN_TYPE PLUGIN_TYPE_DATA

////////////////////////////////////////
// Data section
////////////////////////////////////////
struct PluginInfo oPluginInfo =
{
		sizeof( struct PluginInfo ),
		THIS_PLUGIN_TYPE,		
		PLUGIN_VERSION,
		PLUGIN_ID,
		PLUGIN_NAME,
		VENDOR_NAME,
		13012679,
		387000
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
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return 1;
}	 

PLUGINAPI int Release(void) 
{ 
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return 1; 	  // default implementation does nothing
} 


/////////////////////////////
// Configure function
// is called when user clicks "Configure" button
// in the File->Database Settings dialog
//
// It should show up configuration dialog for the plugin
/////////////////////////
PLUGINAPI int Configure( LPCTSTR pszPath, struct InfoSite *pSite )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	CConfigDlg oDlg;
	oDlg.m_pSite = pSite;

	if( oDlg.DoModal() == IDOK )
	{
	}

	return 1;
}

////////////////////////// 
// GetExtraData function is used
// to retrieve other data (fundamentals for example)
//
// In case of QuoteTracker plugin - it is not used
/////////////
PLUGINAPI AmiVar GetExtraData( LPCTSTR pszTicker, LPCTSTR pszName, int nArraySize, int nPeriodicity, void* (*pfAlloc)(unsigned int nSize) )
{
	AmiVar var;

	var.type = VAR_NONE;
	var.val = 0;

	return var;
}

PLUGINAPI int SetTimeBase( int nTimeBase )
{
	return ( nTimeBase >= 60 && nTimeBase <= ( 24 * 60 * 60 ) )  ? 1 : 0;
}


/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
// Start of data source specific code
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////


/////////////////////////////
// Constants
/////////////////////////////
#define RETRY_COUNT 8
#define AGENT_NAME PLUGIN_NAME
enum
{
	STATUS_WAIT,
	STATUS_CONNECTED,
	STATUS_DISCONNECTED,
	STATUS_SHUTDOWN
};

///////////////////////////////
// Globals
///////////////////////////////

typedef CArray< struct Quotation, struct Quotation > CQuoteArray;

HWND g_hAmiBrokerWnd = NULL;

int		g_nPortNumber = 16239;
int		g_nRefreshInterval = 1;
BOOL	g_bAutoAddSymbols = TRUE;
int		g_nSymbolLimit = 100;
int		g_bOptimizedIntraday = TRUE;
int		g_nTimeShift = 0;
CString g_oServer = "127.0.0.1";

int		g_nStatus = STATUS_WAIT;
int		g_nRetryCount = RETRY_COUNT;

static struct RecentInfo *g_aInfos = NULL;
static int	  RecentInfoSize = 0;



////////////////////////////////////////
// GetSymbolLimit function is called by AmiBroker
// to find out the maximum number of real-time streaming symbols
// that can be displayed in the real-time quote window.
///////////////////////////////////////
PLUGINAPI int GetSymbolLimit( void )
{
	return g_nSymbolLimit;	
}


////////////////////////////////////////
// GetStatus function is called periodically
// (in on-idle processing) to retrieve the status of the plugin
// Returned status information (see PluginStatus structure definition)
// contains numeric status code	as well as short and long
// text descriptions of status.
//
// The highest nibble (4-bit part) of nStatus code
// represents type of status:
// 0 - OK, 1 - WARNING, 2 - MINOR ERROR, 3 - SEVERE ERROR
// that translate to color of status area:
// 0 - green, 1 - yellow, 2 - red, 3 - violet 

PLUGINAPI int GetStatus( struct PluginStatus *status )
{
	switch( g_nStatus )
	{
	case STATUS_WAIT:
		status->nStatusCode = 0x10000000;
		strcpy( status->szShortMessage, "WAIT" );
		strcpy( status->szLongMessage, "Waiting for connection" );
		status->clrStatusColor = RGB( 255, 255, 0 );
		break;
	case STATUS_CONNECTED:
		status->nStatusCode = 0x00000000;
		strcpy( status->szShortMessage, "OK" );
		strcpy( status->szLongMessage, "Connected OK" );
		status->clrStatusColor = RGB( 0, 255, 0 );
		break;
	case STATUS_DISCONNECTED:
		status->nStatusCode = 0x20000000;
		strcpy( status->szShortMessage, "ERR" );
		strcpy( status->szLongMessage, "Disconnected.\n\nPlease check if QuoteTracker is running.\nAmiBroker will try to reconnect in 15 seconds." );
		status->clrStatusColor = RGB( 255, 0, 0 );
		break;
	case STATUS_SHUTDOWN:
		status->nStatusCode = 0x30000000;
		strcpy( status->szShortMessage, "DOWN" );
		strcpy( status->szLongMessage, "Connection is shut down.\nWill not retry until you re-connect manually." );
		status->clrStatusColor = RGB( 192, 0, 192 );
		break;
	default:
		strcpy( status->szShortMessage, "Unkn" );
		strcpy( status->szLongMessage, "Unknown status" );
		status->clrStatusColor = RGB( 255, 255, 255 );
		break;
	}

	return 1;
}

// Quote Tracker reports all times in US eastern time
// this is not a problem for intraday because
// AmiBroker supports time shifting 
// File->Database Settings->Intraday Settings
// but the problem exists for EOD
// quotes when QT reports' previous day for Australia for example

int GetTimeOffset( void )
{
	// set offset initially to 5 hours (difference between GMT and EST)
	int nOffset = 5;

	TIME_ZONE_INFORMATION tzinfo;
	DWORD dw = GetTimeZoneInformation(&tzinfo);
	if (dw == 0xFFFFFFFF)
	{
		return -1;
	}

	if (dw == TIME_ZONE_ID_DAYLIGHT)
		nOffset -= ((tzinfo.Bias + tzinfo.DaylightBias))/60;
	else if (dw == TIME_ZONE_ID_STANDARD)
		nOffset -= ((tzinfo.Bias + tzinfo.StandardBias))/60;
	else
		nOffset -= (tzinfo.Bias)/60;

	return nOffset;
}

////////////////////
// IsQuoteTrackerRunning
// is a helper function that detects if QuoteTracker
// window is open
////////////////////
BOOL IsQuoteTrackerRunning( void )
{
	HANDLE hMutex = CreateMutex( NULL, FALSE, "QuoteTrkr" );

	BOOL bOK =  GetLastError() == ERROR_ALREADY_EXISTS;

	if( hMutex )
	{
		CloseHandle( hMutex );
	}


	return bOK;
}



//////////////////////
// GrowRecentInfoIfNecessary function
// checks the size of RecentInfo table and
// grows the table if needed
//////////////////////

void GrowRecentInfoIfNecessary( int iSymbol )
{
	if( g_aInfos == NULL || iSymbol >= RecentInfoSize )
	{
		RecentInfoSize += 200;
		g_aInfos = (struct RecentInfo *)realloc( g_aInfos, sizeof( struct RecentInfo ) * RecentInfoSize );

		memset( g_aInfos + RecentInfoSize - 200, 0, sizeof( struct RecentInfo ) * 200 );
	}
}

///////////////////////
// FindRecentInfo function
// searches for RecentInfo of requested ticker 
// and returns the pointer to the array element
// holding data for given ticker 
//
// Note that this function performs linear scan
// that is not optimal in performance terms, but
// since we don't track more than a few hundred symbols
// with QuoteTracker this does not affect the performance
// that much considering the fact that it is called only
// a few times per second.
struct RecentInfo *FindRecentInfo( LPCTSTR pszTicker )
{
	struct RecentInfo *ri = NULL;

	for( int iSymbol = 0; g_aInfos && iSymbol < RecentInfoSize && g_aInfos[ iSymbol ].Name && g_aInfos[ iSymbol ].Name[ 0 ]; iSymbol++ )
	{
		if( ! stricmp( g_aInfos[ iSymbol ].Name, pszTicker ) )
		{
			ri = &g_aInfos[ iSymbol ];
			break;
		}
	}

	return ri;
}


////////////////////////////
// AddToQTPortfolio function
// adds specified ticker to the QuoteTracker current portfolio
///////////////////////////

BOOL AddToQTPortfolio( LPCTSTR pszTicker )
{
	BOOL bOK = TRUE;

	try
	{
		CString oURL;
		oURL.Format( "http://%s:%d/req?AddStocksToPort(CURRENT,%s)", g_oServer, g_nPortNumber, pszTicker );

		CInternetSession oSession( AGENT_NAME, 1, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_DONT_CACHE );
		CStdioFile *poFile = oSession.OpenURL( oURL,1, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE );

		CString oLine;
		if( poFile && poFile->ReadString( oLine ) && oLine.Left( 2 ) == "OK" ) bOK = TRUE;


		poFile->Close();

		delete poFile;

		oSession.Close();
	}
	catch( CInternetException *e )
	{
		e->Delete();

		g_nStatus = STATUS_DISCONNECTED;
	}

	return bOK;
}


////////////////////////////
// GetAvailableSymbols function
// retrieves the list of symbols present and active
// in all QuoteTracker portfolios
// This function is called when user clicks
// "Retrieve Symbols" button in Configure dialog
/////////////////////////////
CString GetAvailableSymbols( void )
{
	BOOL bOK = TRUE;

	CString oResult;

	try
	{
		CString oURL;
		oURL.Format( "http://%s:%d/req?EnumSymbols(ACTIVE)", g_oServer, g_nPortNumber );

		CInternetSession oSession( AGENT_NAME, 1, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_DONT_CACHE );
		CStdioFile *poFile = oSession.OpenURL( oURL,1, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE );

		CString oLine;
		if( poFile && poFile->ReadString( oLine ) && oLine.Left( 2 ) == "OK" ) 
		{
			bOK = TRUE;

			poFile->ReadString( oResult );
		}

		poFile->Close();

		delete poFile;

		oSession.Close();
	}
	catch( CInternetException *e )
	{
		e->Delete();

		g_nStatus = STATUS_DISCONNECTED;
	}

	return oResult;
}

////////////////////////////
// FindOrAddRecentInfo
// searches RecentInfo table for record corresponding
// to requested ticker and if it is not found
// it adds new record at the end of the table
/////////////////////////////
struct RecentInfo *FindOrAddRecentInfo( LPCTSTR pszTicker )
{
	struct RecentInfo *ri = NULL;

	if( g_aInfos == NULL ) return NULL;

	for( int iSymbol = 0; g_aInfos && iSymbol < RecentInfoSize && g_aInfos[ iSymbol ].Name && g_aInfos[ iSymbol ].Name[ 0 ]; iSymbol++ )
	{
		if( ! stricmp( g_aInfos[ iSymbol ].Name, pszTicker ) )
		{
			ri = &g_aInfos[ iSymbol ];
			return ri;	// already exists - do not need to add anything
		}
	}

	if( iSymbol < g_nSymbolLimit )
	{
		if( AddToQTPortfolio( pszTicker ) )
		{
			GrowRecentInfoIfNecessary( iSymbol );
			ri = &g_aInfos[ iSymbol ];
			strcpy( ri->Name, pszTicker );
			ri->nStatus = 0;
		}
	}

	return ri;
}

/////////////////////////////////
// Forward declaration of Timer callback function
//////////////////////////////////
VOID CALLBACK OnTimerProc( HWND, UINT, UINT_PTR, DWORD );


///////////////////////////////
// SetupRetry function
// is a helper function that controls
// retries when connect attempt fails.
// After g_nRetryCount attempts it switches to SHUTDOWN state
///////////////////////////////

void SetupRetry( void )
{
	if( --g_nRetryCount )
	{
		SetTimer( g_hAmiBrokerWnd, 198, 15000, (TIMERPROC)OnTimerProc );
		g_nStatus = STATUS_DISCONNECTED;
	}
	else
	{
		g_nStatus = STATUS_SHUTDOWN;
	}
}

//////////////////////////////
// safe_atoi and safe_atof
// are helper function that work the same as
// atoi and atof but do not produce
// crash when NULL string pointer is passed
// as parameter
/////////////////////////////

int	safe_atoi( const char *string )
{
	if( string == NULL ) return 0;

	return atoi( string );
}

double safe_atof( const char *string )
{
	if( string == NULL ) return 0.0f;

	return atof( string );
}

//////////////////////////////
// Timer Callback procedure
// It is called periodically to retrieve
// current quotes from QuoteTracker.
// It just issues request getLastQuote(ACTIVE)
// to QT HTTP server, reads the response
// and updates corresponding fields of recent info table
/////////////////////////////
 
VOID CALLBACK OnTimerProc(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT_PTR idEvent,  // timer identifier
  DWORD dwTime       // current system time
  )
{
	struct RecentInfo *ri;

	if( idEvent == 199 || idEvent == 198 )
	{
		if( ! IsQuoteTrackerRunning() )
		{
			KillTimer( g_hAmiBrokerWnd, idEvent );
			SetupRetry();
			return;
		}
		
		try
		{
			CInternetSession oSession( AGENT_NAME, 1, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_DONT_CACHE );
			
			CString oURL;

			oURL.Format("http://%s:%d/req?getLastQuote(ACTIVE)", g_oServer, g_nPortNumber );

			CStdioFile *poFile = oSession.OpenURL( oURL,1, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE );

			if( poFile )
			{
				CString oLine;

				int iSymbol = 0;

				if( poFile->ReadString( oLine ) )
				{
					if( oLine == "OK" )
					{
						while( poFile->ReadString( oLine ) )
						{
							TRACE( oLine + "\n" );
							
							if( oLine.GetLength() < 20 ) continue;//AfxMessageBox("test");

							GrowRecentInfoIfNecessary( iSymbol );

							ri = &g_aInfos[ iSymbol ];
									
							char *line = oLine.LockBuffer();


							ri->nStructSize = sizeof( struct RecentInfo );
							strcpy( ri->Name, strtok( line, "," ) );

							int month = safe_atoi( strtok( NULL, "/-." ) );
							int day = safe_atoi( strtok( NULL, "/-." ) );
							int year = safe_atoi( strtok( NULL, "," ) );

							int hour = safe_atoi( strtok( NULL, ":." ) );
							int minute = safe_atoi( strtok( NULL, ":." ) );
							int second = safe_atoi( strtok( NULL, "," ) );

							float fOldLast, fOldBid, fOldAsk;

							fOldLast = ri->fLast;
							fOldBid = ri->fBid;
							fOldAsk = ri->fAsk;

							//->nTimeUpdate = ri->nTimeUpdate & ~0x07 + time(NULL) & 0x7;


							ri->fLast = (float) safe_atof( strtok( NULL, "," ) );
							ri->fBid = (float) safe_atof( strtok( NULL, "," ) );
							ri->fAsk = (float) safe_atof( strtok( NULL, "," ) );
							ri->fChange = (float) safe_atof( strtok( NULL, "," ) );

							// tick
							strtok( NULL, "," );

							double dTemp = safe_atof( strtok( NULL, "," ) ); 
							ri->iTotalVol = (int) dTemp;
							ri->fTotalVol = (float) dTemp;
 							ri->fHigh = (float) safe_atof( strtok( NULL, "," ) );
							ri->fLow = (float) safe_atof( strtok( NULL, "," ) );
							ri->iBidSize = safe_atoi( strtok( NULL, "," ) );
							ri->iAskSize = safe_atoi( strtok( NULL, "," ) );

							dTemp = safe_atof( strtok( NULL, "," ) ); 
							ri->iTradeVol = (int) dTemp;
							ri->fTradeVol = (float) dTemp;

							strtok( NULL, "," );
							strtok( NULL, "," );
							ri->fOpen = (float) safe_atof( strtok( NULL, "," ) );
							ri->f52WeekLow = (float) safe_atof( strtok( NULL, "," ) );
							ri->f52WeekHigh = (float) safe_atof( strtok( NULL, "," ) );


							ri->nDateChange = 10000 * year + 100 * month + day; 
							ri->nTimeChange = 10000 * hour + 100 * minute + second;

							if( ri->fLast != fOldLast || ri->fAsk != fOldAsk || ri->fBid != fOldBid )
							{
								ri->nDateUpdate = ri->nDateChange;
								ri->nTimeUpdate = ri->nTimeChange;
							}

							ri->nBitmap = RI_LAST | ( ri->fOpen ? RI_OPEN : 0 ) | ( ri->fHigh ? RI_HIGHLOW : 0 ) | RI_TRADEVOL | RI_52WEEK | 
								RI_TOTALVOL | RI_PREVCHANGE | RI_BID | RI_ASK | RI_DATEUPDATE | RI_DATECHANGE;	

							ri->nStatus = RI_STATUS_UPDATE | RI_STATUS_BIDASK | RI_STATUS_TRADE | RI_STATUS_BARSREADY;


							oLine.UnlockBuffer();

							iSymbol++;
						}
					}
				}

				::SendMessage( g_hAmiBrokerWnd, WM_USER_STREAMING_UPDATE, 0, 0 );

				poFile->Close();
				delete poFile;
		
			
				g_nStatus = STATUS_CONNECTED;
				g_nRetryCount = RETRY_COUNT;
			}

			oSession.Close();
		}
		catch( CInternetException *e )
		{
			e->Delete();
			KillTimer( g_hAmiBrokerWnd, idEvent );
			SetupRetry();
			return;
		} 

	}

	if( idEvent == 198 )
	{
		KillTimer( g_hAmiBrokerWnd, 198 );
		SetTimer( g_hAmiBrokerWnd, 199, g_nRefreshInterval * 1000, (TIMERPROC)OnTimerProc );
	}
}

///////////////////////////////
// GetIntradayBars() function
// retrieves Time/Sales records starting from nStartTime from QuoteTracker HTTP server
// and compresses them into N-minute records of 
// requested periodicity (interval).
///////////////////////////////
BOOL GetIntradayBars( LPCTSTR pszTicker, int nPeriodicity, int nStartTime, CQuoteArray *pCurQuotes )
{
	BOOL bResult = FALSE;
	
	try
	{
		CInternetSession oSession( AGENT_NAME, 1, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_DONT_CACHE );
		
		CString oURL;

		oURL.Format("http://%s:%d/req?GetTimeSales(%s,%d,0)", g_oServer, g_nPortNumber, pszTicker, nStartTime );

		CStdioFile *poFile = oSession.OpenURL( oURL,1, INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE );

		if( poFile )
		{
			CString oLine;

			int iSymbol = 0;


			if( poFile->ReadString( oLine ) )
			{
				if( oLine == "OK" )
				{
					union AmiDate lastdate;
					union AmiDate curdate;

					lastdate.Date = curdate.Date = 0;

					float fCumVolume = 0;
					float fPrevCumVolume = 0;

					while( poFile->ReadString( oLine ) )
					{
						TRACE( oLine + "\n" );

						char *line = oLine.LockBuffer();

						curdate.PackDate.Month = safe_atoi( strtok( line, "/-." ) );
						curdate.PackDate.Day = safe_atoi( strtok( NULL, "/-." ) );
						curdate.PackDate.Year = safe_atoi( strtok( NULL, "," ) );

						curdate.PackDate.Hour = atoi( strtok( NULL, ":." ) );
						curdate.PackDate.Minute = atoi( strtok( NULL, ":." ) );
						curdate.PackDate.Second = 0;	// 

						if( nPeriodicity > 60 && nPeriodicity < 3600 )
						{
							curdate.PackDate.Minute = (nPeriodicity/60) * ( curdate.PackDate.Minute / (nPeriodicity/60) );
						}

						if( nPeriodicity == 3600 ) curdate.PackDate.Minute = 0;
						
						strtok( NULL, "," );		// skip seconds - we are using only minutes

						float fLast = (float) atof( strtok( NULL, "," ) );

						strtok( NULL, "," );		// skip Ask
						strtok( NULL, "," );		// skip Bid

						float fVolume = (float) atof( strtok( NULL, "," ) );

						fCumVolume += fVolume;

						fVolume = max( 0, fCumVolume - fPrevCumVolume );

						if( lastdate.Date < curdate.Date )
						{
							lastdate.Date = curdate.Date;

							struct Quotation NewQT;
							NewQT.DateTime = curdate;
							NewQT.Open = NewQT.High = NewQT.Low = NewQT.Price = fLast;
							NewQT.Volume = fVolume;
							NewQT.OpenInterest = NewQT.AuxData1 = NewQT.AuxData2 = 0;

							pCurQuotes->Add( NewQT );

						}
						else
						{
							struct Quotation *qt = & pCurQuotes->GetData()[ pCurQuotes->GetUpperBound() ];

							qt->Price = fLast;
							qt->Volume += fVolume;
							if( qt->High < fLast ) qt->High = fLast;
							if( qt->Low > fLast ) qt->Low = fLast;
						}
							
						oLine.UnlockBuffer();

						fPrevCumVolume = fCumVolume;

					}

					bResult = TRUE;

				}
			}

			poFile->Close();
			delete poFile;

		}

		oSession.Close();
	}
	catch( CInternetException *e )
	{
		e->Delete();
		g_nStatus = STATUS_DISCONNECTED;
	} 

	return bResult;
}

/////////////////////////////////////////
// BlendQuoteArrays is general-purpose function
// (not data-source dependent) that merges
// quotes that are already present in AmIBroker's database
// with new quotes retrieved from the data source
/////////////////////////////////////////

int BlendQuoteArrays( struct Quotation *pQuotes, int nPeriodicity, int nLastValid, int nSize, CQuoteArray *pCurQuotes )
{
	int iQty = pCurQuotes->GetSize();

	DATE_TIME_INT nFirstDate = iQty == 0 ? (DATE_TIME_INT)-1 : pCurQuotes->GetAt( 0 ).DateTime.Date;

	for( int iStart = nLastValid; iStart >= 0; iStart-- )
	{
		// find last which is not greater than the one we
		// have from real time
		if( pQuotes[ iStart ].DateTime.Date < nFirstDate ) break;
	}

	iStart++; // start with next

	int iSrc = 0; // by default we start with the first bar we have

	// if we have more RT data than requested
	// we fill entire array
	// and start with the first we can
	//
	// <------------------------------------------> nSize
	// <------------------------------------------------> iQty
	//       ^ we have to start here iStart
	//
	if( iQty > nSize )
	{
		iStart = 0; 
		iSrc = iQty - nSize;
	}
	else
	if( iQty + iStart > nSize )
	{
		// Now handle the situation like this:
		// <------------------------------------------> nSize
		//		<-------------------------------------> iQty
		// <------>
		//		   iStart

		memmove( pQuotes, pQuotes + iQty + iStart - nSize, sizeof( Quotation ) * ( nSize - iQty ) );
	
		iStart = nSize - iQty;
		iSrc = 0;
	}

	int iNumQuotes = min( nSize - iStart, iQty - iSrc );	

	if( iNumQuotes > 0 )
	{
		memcpy( pQuotes + iStart, pCurQuotes->GetData() + iSrc, iNumQuotes * sizeof( Quotation ) );
	}
	else
	{
		iNumQuotes = 0;
	}

	return iStart + iNumQuotes;

}



///////////////////////////
// This function adjusts date and time
// according to time zone shift - this is needed
// because QuoteTracker returns all times in EST time zone.
int GetAdjustedDate( int nDate, int nTime )
{
	int nYear = (nDate / 10000);
	int nMonth = (nDate / 100 ) % 100;
	int nDay = nDate % 100;

	int nHour = (nTime / 10000);
	int nMinute = (nTime / 100 ) % 100;
	int nSecond = nTime % 100;

	CTime oTime( nYear, nMonth, nDay, nHour, nMinute, nSecond );

	oTime += CTimeSpan( 0, g_nTimeShift, 0, 0 );

	return 10000 * oTime.GetYear() + 100 * oTime.GetMonth() + oTime.GetDay();

}


// LEGACY format support
// convert back and forth between old and new format
//
// WARNING: it is highly inefficient and should be avoided
// So this is left just for maintaning compatibility,
// not for performance

PLUGINAPI int GetQuotes( LPCTSTR pszTicker, int nPeriodicity, int nLastValid, int nSize, struct QuotationFormat4 *pQuotes  )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	Quotation *pQuote5 = (struct Quotation *) malloc( nSize * sizeof( Quotation ) );

	QuotationFormat4 *src = pQuotes; 
	Quotation *dst = pQuote5;

	int i;

	for( i = 0; i <= nLastValid; i++, src++, dst++ )
	{
		ConvertFormat4Quote( src, dst );
	}

	int nQty = GetQuotesEx( pszTicker, nPeriodicity, nLastValid, nSize, pQuote5, NULL );

	dst = pQuote5;
	src = pQuotes;

	for( i = 0; i < nQty; i++, dst++, src++ )
	{
		ConvertFormat5Quote( dst, src );
	}

	free( pQuote5 );

	return nQty;
}

//////////////////////////////////////
// GetQuotesEx is a most important function
// for every data plugin
// it is called by AmiBroker everytime AmiBroker
// needs new data for given symbol.
//
// Internally AmiBroker caches response obtained
// from GetQuotes function but you may force it to 
// get new data by sending appropriate message to AmiBroker
// main window.
//
//
// When AmiBroker calls GetQuotes function it allocates
// array of quotations of size equal to default number of bars as set in
// File->Database Settings,
// and fills the array with quotes that are already present in the
// database.
// Filled area covers array elements from zero to nLastValid
//
// In your DLL you can update the array with more recent quotes.
// Depending on the data source you can either fill entire array
// from the scratch (Metastock, TC2000, QP2 plugins do that)
// or just add/update a few recent bars and leave the remaining bars
// untouched (eSignal, myTrack, QuoteTracker plugins do that)

PLUGINAPI int GetQuotesEx( LPCTSTR pszTicker, int nPeriodicity, int nLastValid, int nSize, struct Quotation *pQuotes, GQEContext *pContext  )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	if( g_nStatus >= STATUS_DISCONNECTED ) return nLastValid + 1;

	struct RecentInfo *ri;
	
	if( g_bAutoAddSymbols )
		ri = FindOrAddRecentInfo( pszTicker );
	else
		ri = FindRecentInfo( pszTicker );

	if( nPeriodicity == 24 * 60 * 60 ) // daily
	{
		if( ri && nSize > 0 && ri->nStatus && ri->fLast )
		{
			BOOL bTooOld = FALSE;

			int nCurDate = GetAdjustedDate( ri->nDateUpdate, ri->nTimeUpdate );

			if( nLastValid >= 0 )
			{
				int nLastDate = ( pQuotes[ nLastValid ].DateTime.PackDate.Year ) * 10000	+
								pQuotes[ nLastValid ].DateTime.PackDate.Month * 100 +
								pQuotes[ nLastValid ].DateTime.PackDate.Day;


				if( nCurDate < nLastDate ) bTooOld = TRUE; 
				if( nCurDate > nLastDate ) nLastValid++;
			}
			else
			{
				nLastValid = 0;
			}
			
			if( nSize > 0 && nLastValid == nSize ) 
			{
				memmove( pQuotes, pQuotes + 1, ( nSize - 1 ) * sizeof (struct Quotation ) );
				nLastValid--;
			}

			if( ! bTooOld )
			{
				struct Quotation *qt = &pQuotes[ nLastValid ];

				qt->DateTime.Date = DAILY_MASK;
				qt->DateTime.PackDate.Year = (nCurDate / 10000) ;
				qt->DateTime.PackDate.Month = (nCurDate / 100 ) % 100;
				qt->DateTime.PackDate.Day = nCurDate % 100;

				qt->Open = ri->fOpen ? ri->fOpen : ri->fLast;
				qt->Price = ri->fLast;
				qt->High = ri->fHigh ? ri->fHigh : ri->fLast;
				qt->Low = ri->fLow ? ri->fLow : ri->fLast;
				qt->Volume = ri->fTotalVol;
				qt->OpenInterest = 0;
				qt->AuxData1 = 0;
				qt->AuxData2 = 0;
			}
		}

		return nLastValid + 1;
	}
	else
	{
		static AmiDate oToday;
	
		CQuoteArray oCurQuotes;

		if( oToday.Date == 0 )
		{
			//
			// get last trading day date
			//
			SYSTEMTIME oTime;
			GetSystemTime( &oTime );

			if( oTime.wDayOfWeek == 0 && oTime.wDay > 2 ) oTime.wDay -=2; /* sunday */
			if( oTime.wDayOfWeek == 6 && oTime.wDay > 1 ) oTime.wDay -=1; /* saturday */

			oToday.PackDate.Year = oTime.wYear;
			oToday.PackDate.Month = oTime.wMonth;
			oToday.PackDate.Day = oTime.wDay;
		}
			
		int nStartTime = 0;

		static int iAccessCounter = 0;

		// 
		// This strange looking code is provided because QuoteTracker
		// generates bar for non trading hours that can fool the checking routine
		// So every 16-th refresh we are checking 100 bars back (instead of 4)
		// if we have data for today or yesterday
		// 
		int iCheckBack = ( (iAccessCounter++) & 0xF ) ? 4 : 100;

		if( g_bOptimizedIntraday && nLastValid > iCheckBack )
		{
			AmiDate oBarDate = pQuotes[ nLastValid - iCheckBack ].DateTime;

			if( ( ( oBarDate.Date & DAILY_MASK ) != DAILY_MASK ) && /* not EOD quote */
				( oBarDate.Date & ~DAILY_MASK ) == oToday.Date /* intraday for current day */ )
			{
				nStartTime = 10000 * oBarDate.PackDate.Hour + 100 * oBarDate.PackDate.Minute;
			}
		}			

		GetIntradayBars( pszTicker, nPeriodicity, nStartTime, &oCurQuotes );
	
		return BlendQuoteArrays( pQuotes, nPeriodicity, nLastValid, nSize, &oCurQuotes );
	}
}


///////////////////////////////////////
// Notify function implementation
//
// Notify() is called by AmiBroker
// in the various circumstances including:
// 1. database is loaded/unloaded
// 2. right mouse button is clicked in the 
//    plugin status area of AmiBroker's status bar.
//
// The implementation may provide special handling
// of such events.
///////////////////////////////////////

PLUGINAPI int Notify(struct PluginNotification *pn) 
{ 
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	if( g_hAmiBrokerWnd == NULL && ( pn->nReason & REASON_DATABASE_LOADED ) )
	{
		g_hAmiBrokerWnd = pn->hMainWnd;

		g_nTimeShift = GetTimeOffset();

		g_nTimeShift = AfxGetApp()->GetProfileInt( "QuoteTracker", "TimeShift", g_nTimeShift );
		TRACE("TIME OFFSET %d\n", g_nTimeShift );

		g_oServer = AfxGetApp()->GetProfileString( "QuoteTracker", "Server", "127.0.0.1" );
		g_nPortNumber = AfxGetApp()->GetProfileInt( "QuoteTracker", "Port", 16239 );
		g_nRefreshInterval = AfxGetApp()->GetProfileInt( "QuoteTracker", "RefreshInterval", 5 );
		g_bAutoAddSymbols = AfxGetApp()->GetProfileInt( "QuoteTracker", "AutoAddSymbols", 1 );
		g_nSymbolLimit = AfxGetApp()->GetProfileInt( "QuoteTracker", "SymbolLimit", 100 );
		g_bOptimizedIntraday = AfxGetApp()->GetProfileInt( "QuoteTracker", "OptimizedIntraday", 1 );

		g_nStatus = STATUS_WAIT;

		SetTimer( g_hAmiBrokerWnd, 198, 1000, (TIMERPROC)OnTimerProc );

	}

	if( pn->nReason & REASON_DATABASE_UNLOADED ) 
	{
		KillTimer( g_hAmiBrokerWnd, 198 );
		KillTimer( g_hAmiBrokerWnd, 199 );
		g_hAmiBrokerWnd = NULL;

		free( g_aInfos );
		g_aInfos = NULL;
	}


	if( pn->nReason & REASON_STATUS_RMBCLICK ) 
	{
		CPoint pt;

		GetCursorPos( &pt );

		CMenu oMenu; 
		CMenu *poPopup = NULL;

		oMenu.LoadMenu( IDR_STATUS_MENU );

		poPopup = oMenu.GetSubMenu( 0 );

		if( g_nStatus == STATUS_CONNECTED )
		{
			poPopup->EnableMenuItem(ID_STATUS_RECONNECT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED ); 
		}

		if( g_nStatus == STATUS_SHUTDOWN )
		{
			poPopup->EnableMenuItem(ID_STATUS_SHUTDOWN, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED ); 
		}

		//
		// Show popup window
		// 
		// Note: specyfing 	TPM_NONOTIFY is ESSENTIAL - otherwise it crashes !!!
		// because of the fact that cool menus look for accellerators and this fails
		// when resources are switched to DLL ones.
		//

		int nResponse =  poPopup->TrackPopupMenu( TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, CWnd::FromHandle( pn->hMainWnd ) );

		switch( nResponse )
		{
		case ID_STATUS_RECONNECT:
			KillTimer( g_hAmiBrokerWnd, 198 );
			KillTimer( g_hAmiBrokerWnd, 199 );
			SetTimer( g_hAmiBrokerWnd, 198, 1000, (TIMERPROC)OnTimerProc );
			break;
		case ID_STATUS_SHUTDOWN:
			KillTimer( g_hAmiBrokerWnd, 198 );
			KillTimer( g_hAmiBrokerWnd, 199 );
			g_nStatus = STATUS_SHUTDOWN;
			break;
		}
	
	}

	return 1;
}	 

///////////////////////////////////////
// GetRecentInfo function is called by
// real-time quote window to retrieve the most
// recent quote and other data
// see the definition of RecentInfo structure
// in the Plugin.h file
///////////////////////////////////////

PLUGINAPI struct RecentInfo * GetRecentInfo( LPCTSTR pszTicker )
{
	return FindRecentInfo( pszTicker );
}

