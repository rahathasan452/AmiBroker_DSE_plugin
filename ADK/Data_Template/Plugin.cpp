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
#include "Test.h"
#include "Plugin.h"
#include "resource.h"
#include "ConfigDlg.h"


// These are the only two lines you need to change
#define PLUGIN_NAME "Your own data Plug-in"
#define VENDOR_NAME "Your company name here"
#define PLUGIN_VERSION 10000
#define PLUGIN_ID PIDCODE( 'T', 'E', 'S', 'T')

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
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return 1;
}	 

PLUGINAPI int Release(void) 
{ 
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	return 1; 	  // default implementation does nothing
} 



PLUGINAPI int Notify(struct PluginNotification *pn) 
{ 
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

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
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	//////////////////////////////////////////////////////////////////
	// TODO: Insert code that gets quotes from your data source here
	/////////////////////////////////////////////////////////////////

	// default implementation returns the size of array
	// as received from AmiBroker
	return nLastValid + 1;
}


PLUGINAPI int Configure( LPCTSTR pszPath, struct InfoSite *pSite )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	// show Configure dialog
	CConfigDlg oDlg;
	oDlg.DoModal();

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
	// allow all intervals from 1-minute bars upto daily bars
	return ( nTimeBase >= 60 && nTimeBase <= ( 24 * 60 * 60 ) )  ? 1 : 0;
}

