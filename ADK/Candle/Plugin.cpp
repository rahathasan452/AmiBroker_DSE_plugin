////////////////////////////////////////////////////
// Plugin.cpp
// Standard implementation file for all AmiBroker plug-ins
//
// Copyright (C)2001 Tomasz Janeczko, amibroker.com
// All rights reserved.
//
// Last modified: 2001-09-24 TJ
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
#include "Plugin.h"

// These are the only two lines you need to change
#define PLUGIN_NAME "Candlestick function plug-in"
#define VENDOR_NAME "Amibroker.com"
#define PLUGIN_VERSION 000100

////////////////////////////////////////
// Data section
////////////////////////////////////////
static struct PluginInfo oPluginInfo =
{
		sizeof( struct PluginInfo ),
		1,		
		PLUGIN_VERSION,
		0,
		PLUGIN_NAME,
		VENDOR_NAME,
		13207239,
		371000
};

// the site interface for callbacks
struct SiteInterface gSite;

///////////////////////////////////////////////////////////
// Basic plug-in interface functions exported by DLL
///////////////////////////////////////////////////////////

PLUGINAPI int GetPluginInfo( struct PluginInfo *pInfo ) 
{ 
	*pInfo = oPluginInfo;

	return TRUE;
}


PLUGINAPI int SetSiteInterface( struct SiteInterface *pInterface )
{
	gSite = *pInterface;

	return TRUE;
}


PLUGINAPI int GetFunctionTable( FunctionTag **ppFunctionTable )
{
	*ppFunctionTable = gFunctionTable;

	// must return the number of functions in the table
	return gFunctionTableSize;
}

PLUGINAPI int Init(void) 
{ 
	return 1; 	 // default implementation does nothing

};	 

PLUGINAPI int Release(void) 
{ 
	return 1; 	  // default implementation does nothing
}; 



