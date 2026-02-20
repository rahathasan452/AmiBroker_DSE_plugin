////////////////////////////////////////////////////
// Plugin.cpp
// Standard implementation file for all AmiBroker plug-ins
//
// Copyright (C)2001,2008 Tomasz Janeczko, amibroker.com
// All rights reserved.
//
// Last modified: 2008-06-17 TJ
// Created:		  2001-12-28 TJ
// 
// You may use this code in your own projects provided that:
//
// 1. You are registered user of AmiBroker
// 2. Your project will be used only in conjunction with AmiBroker
// 3. You keep all original copyright notices in the source code 
//
//
////////////////////////////////////////////////////

#include "stdafx.h"
#include "Plugin.h"
#include "resource.h"
#include <math.h>

#include "TribesD.h" // headers for global stuff used in SPSO2007.cpp


// These are the only two lines you need to change
#define PLUGIN_NAME "TRIBES Optimizer plug-in"
#define VENDOR_NAME "AmiBroker.com"
#define PLUGIN_VERSION 10001


#define THIS_PLUGIN_TYPE PLUGIN_TYPE_OPTIMIZER

////////////////////////////////////////
// Data section
////////////////////////////////////////
static struct PluginInfo oPluginInfo =
{
		sizeof( struct PluginInfo ),
		THIS_PLUGIN_TYPE,		
		PLUGIN_VERSION,
		PIDCODE( 't', 'r', 'i', 'b'),
		PLUGIN_NAME,
		VENDOR_NAME,
		13012679,
		512000
};

struct OptimizeParams *g_pOptParams = NULL;
double (*g_pfEvaluateFunc)( void * ) = NULL;
void   *g_pOptimizerContext = NULL;


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

// OptimizerInit
//
// This function gets called when AmiBroker collected all information
// about parameters that should be optimized
// This information is available in OptimizeParams structure
// 
// The optimization engine should use this point to initialize internal
// data structures. Also the optimizer should set the value of
// pParams->NumSteps variable to the expected TOTAL NUMBER OF BACKTESTS
// that are supposed to be done during optimization.
// This value is used for two purposes:
// 1. progress indicator (total progress is expressed as backtest number divided by NumSteps)
// 2. flow control (by default AmiBroker will continue calling OptimizerRun until
//    number of backtests reaches the NumSteps) - it is possible however to
//    override that (see below)
//   
// Note that sometimes you may not know exact number of steps (backtests) in advance, 
// in that case provide estimate. Later, inside OptimizerRun you will be able to
// adjust it, as tests go by.
//
// Return values:
// 1 - initialization complete and OK
// 0 - init failed

PLUGINAPI int OptimizerInit( struct OptimizeParams *pParams )
{
	pParams->NumSteps = runMax * evalMax; 	 // MUST set the total number of backtests

	g_pOptParams = pParams;

	init();	// see TribesD.cpp for implementation


	return 1;
}

// OptimizerSetOption
// This function is intended to be used to allow setting additional options / parameters
// of optimizer from the AFL level. 
//
// It gets called in two situations:
//
// 1. When SetOptimizerEngine() AFL function is called for particular optimizer
//         - then it calls OptimizerSetOption once with pszParam set to NULL
//           and it means that optimizer should reset parameter values to default values
//
// 2. When OptimizerSetOption( "paramname", value ) AFL function is called
//
// return codes:
// 1 - OK (set successful)
// 0 - option does not exist
// -1 - wrong type, number expected
// -2 - wrong type, string expected

PLUGINAPI int OptimizerSetOption( const char *pszParam, AmiVar newValue )
{

	// if pszParam is NULL
	// it means set DEFAULT values for all parameters
	// dValue is ignored then

	if( pszParam == NULL )
	{
		// set the DEFAULT values
		runMax = 5;
		evalMax = 1000;
		return 1;
	}

	if( newValue.type != VAR_FLOAT ) return -1; // accept only numbers 

	if( stricmp( pszParam, "MaxEval" ) == 0 )
	{
		evalMax = (int) newValue.val;
		return 1;
	}
	else
	if( stricmp( pszParam, "Runs" ) == 0 )
	{
		runMax = (int) newValue.val;
		return 1;
	}

	return 0; // option not found
}

// OptimizerRun
//
// This function is called multiple times during main optimization loop
//
// There are two basic modes of operations
// 1. Simple Mode
// 2. Advanced Mode
//
// In simple optimization mode, AmiBroker calls OptimizerRun before running
// backtest internally. Inside OptimizationRun the plugin should simply
// set current values of parameters and return 1 as long as backtest using
// given parameter set should be performed. AmiBroker internally will
// do the remaining job.
// By default the OptimizerRun will be called pParams->NumSteps times.
// 
// In this mode you don't use pfEvaluateFunc argument. 
// 
// See Monte Carlo (moca) sample optimizer for coding example using simple mode.
//
//
// In advanced optimization mode, you can trigger multiple "objective function"
// evaluations during single OptimizerRun call.
// 
// There are many algorithms (mostly "evolutionary" ones) that perform optimization
// by doing multiple runs, with each run consisting of multiple "objective function"/"fitness"
// evaluations.
// To allow interfacing such algorithms with AmiBroker's optimizer infrastructure
// the advanced mode provides access to pfEvaluateFunc pointer that call evaluation function.
//
// In order to properly evaluate objective function you need to call it the following way:
//
//  pfEvaluateFunc( pContext );
// 
// Passing the pContext pointer is absolutely necessary as it holds internal state of AmiBroker
// optimizer. The function will crash if you fail to pass the context.
//
// The following things happen inside AmiBroker when you call evaluation function:
// a) the backtest with current parameter set (stored in pParams) is performed
// b) step counter gets incremented (pParams->Step)
// c) progress window gets updated
// d) selected optimization target value is calculated and stored in pParams->TargetCurrent
//    and returned as a result of pfEvaluateFunc
//
// Once you call pfEvaluateFunc() from your plugin, AmiBroker will know that
// you are using advanced mode, and will NOT perform extra backtest after returning
// from OptimizerRun
//
// By default AmiBroker will continue to call OptimizerRun as long as pParams->Step reaches
// pParams->NumSteps. You can overwrite this behaviour by returning value other than 1.
//
// Return values:
// 0 - terminate optimization
// 1 (default value) - optimization should continue until reaching defined number of steps
// 2 - continue optimization loop regardless of step counter

PLUGINAPI int OptimizerRun( struct OptimizeParams *pParams, double (*pfEvaluateFunc)( void * ), void *pContext )
{
	// setting these global variables
	// for later use in SPSO2007.cpp
	g_pOptParams = pParams;
	g_pfEvaluateFunc = pfEvaluateFunc;
	g_pOptimizerContext = pContext;

	return optimize(); // continue until all steps are done
}

// OptimizerFinalize
//
// This function gets called when AmiBroker has completed the optimization
// 
// The optimization engine should use this point to release internal
// data structures. 
// 
// Return values:
// 1 - finalization complete and OK
// 0 - finalization failed

PLUGINAPI int OptimizerFinalize( struct OptimizeParams *pParams )
{
	g_pOptParams = pParams;

	final();  // see TribesD.cpp for implementation

	return 1;

}



