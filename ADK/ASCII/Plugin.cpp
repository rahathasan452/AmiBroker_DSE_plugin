////////////////////////////////////////////////////
// Plugin.cpp
// Standard implementation file for all AmiBroker plug-ins
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
#include "ASCII.h"
#include "Plugin.h"
#include "resource.h"


// These are the only two lines you need to change
#define PLUGIN_NAME "ASCII sample data Plug-in"
#define VENDOR_NAME "Your vendor name"
#define PLUGIN_VERSION 10000
#define PLUGIN_ID PIDCODE( 'A', 'S', 'C', 'I' )

#define THIS_PLUGIN_TYPE PLUGIN_TYPE_DATA

////////////////////////////////////////
// Data section
////////////////////////////////////////
static struct PluginInfo oPluginInfo =
{
		sizeof( struct PluginInfo ),
		THIS_PLUGIN_TYPE,		
		PLUGIN_VERSION,
		PLUGIN_ID,
		PLUGIN_NAME,
		VENDOR_NAME,
		0,
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
	return 1;
}	 

PLUGINAPI int Release(void) 
{ 
	return 1; 	  // default implementation does nothing
} 



PLUGINAPI int Notify(struct PluginNotification *pn) 
{ 
	// default implementation does nothing

	return 1;
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

PLUGINAPI int GetQuotesEx( LPCTSTR pszTicker, int nPeriodicity, int nLastValid, int nSize, struct Quotation *pQuotes, GQEContext *pContext  )
{
	// needed because we will use MFC inside this function
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	// we assume that intraday data files are stored in ASCII subfolder
	// of AmiBroker directory and they have name of <SYMBOL>.AQI
	// and the format of Date(YYMMDD),Time(HHMM),Open,High,Low,Close,Volume
	// and quotes are sorted in ascending order - oldest quote is on the top

	char filename[ 256 ];
	FILE *fh;
	int  iLines = 0;
	
	// format path to the file (we are using relative path)
	sprintf( filename, "ASCII\\%s.AQI", pszTicker );

	// open file for reading
	fh = fopen( filename, "r" );

	// if file is successfully opened read it and fill quotation array
	if( fh )
	{
		char line[ 256 ];

		// read the line of text until the end of text
		// but not more than array size provided by AmiBroker
		while( fgets( line, sizeof( line ), fh ) && iLines < nSize )
		{
			// get array entry
			struct Quotation *qt = &pQuotes[ iLines ];
			
			// parse line contents: divide tokens separated by comma (strtok) and interpret values
			
			// date	and time first
			int datenum = atoi( strtok( line, "," ) );	// YYMMDD
			int timenum = atoi( strtok( NULL, "," ) );	// HHMM

			qt->DateTime.Date = 0; // init structure with zero
			qt->DateTime.PackDate.Minute = timenum % 100;
			qt->DateTime.PackDate.Hour = timenum / 100;
			qt->DateTime.PackDate.Year = 2000 + datenum / 10000;
			qt->DateTime.PackDate.Month = ( datenum / 100 ) % 100;
			qt->DateTime.PackDate.Day = datenum % 100;

			// now OHLC price fields
			qt->Open = (float) atof( strtok( NULL, "," ) );
			qt->High = (float) atof( strtok( NULL, "," ) );
			qt->Low  = (float) atof( strtok( NULL, "," ) );
			qt->Price = (float) atof( strtok( NULL, "," ) ); // close price

			// ... and Volume
			qt->Volume = (float) atof( strtok( NULL, ",\n" ) );

			iLines++;
		}

		// close the file once we are done
		fclose( fh );

	}

	// return number of lines read which is equal to
	// number of quotes
	return iLines;	 
}


PLUGINAPI int Configure( LPCTSTR pszPath, struct InfoSite *pSite )
{
	// AFX_MANAGE_STATE is needed because dialog boxes call MFC and use resources
	// similar line should be added to any other function exported by the plugin
	// if only the function calls MFC or uses DLL resources
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	/*
	** Here a config dialog may be called
	  
	CConfigDlg oDlg;

	oDlg.DoModal();

	**
	*/

	AfxMessageBox("Configure button clicked");

	return 1;
}

PLUGINAPI AmiVar GetExtraData( LPCTSTR pszTicker, LPCTSTR pszName, int nArraySize, int nPeriodicity, void* (*pfAlloc)(unsigned int nSize) )
{
	// default implementation does nothing

	AmiVar var;

	var.type = VAR_NONE;
	var.val = 0;
	
	return var;
}

PLUGINAPI int SetTimeBase( int nTimeBase )
{
	// only intraday intervals are supported
	return ( nTimeBase < ( 24 * 60 * 60 ) )  ? 1 : 0;
}

