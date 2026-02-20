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

#include "cmaes/cmaes_interface.h" // headers CMA-ES engine


// These are the only two lines you need to change
#define PLUGIN_NAME "Covariance Matrix Adaptation ES optimizer plug-in"
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
		PIDCODE( 'c', 'm', 'a', 'e'),
		PLUGIN_NAME,
		VENDOR_NAME,
		13012679,
		512000
};

int evalMax = 0;
int	runMax = 5;
int run = 0;
int lambda = 0;



struct OptimizeParams *g_pOptParams = NULL;
double (*g_pfEvaluateFunc)( void * ) = NULL;
void   *g_pOptimizerContext = NULL;

void init();
int  optimize();
void final();

#define sqr( x ) ((x)*(x))

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
	// 900*(N+3)*(N+3)

	if( evalMax == 0 ) evalMax = 900 * sqr( pParams->Qty + 3 ) ; // theoretical maximum of evaluations

	// estimated evaluation count in first run
	int estEval = 30 * sqr( pParams->Qty + 3 ); 

	if( estEval > evalMax ) estEval = evalMax;

	pParams->NumSteps = (int) ( estEval * pow( 1.55, runMax ) ); // calc estimate of needed steps (will be updated if necessary during eval calls)

	g_pOptParams = pParams;

	run = 0;
	lambda = 0;


	init();	//

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
		evalMax = 0;
		runMax = 5;
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
	g_pOptParams = pParams;
	g_pfEvaluateFunc = pfEvaluateFunc;
	g_pOptimizerContext = pContext;

	return optimize();	 // see below for the implementation

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

	final();  // 

	return 1;

}

//////////////////////////////////
// GLOBAL variables
////////////////////////////////


cmaes_t evo; /* an CMA-ES type struct or "object" */
double *arFunvals, *const*pop, *xfinal;
int i;  
double incpopsize = 2; 

void init( void )
{
	double *initX = new double[ g_pOptParams->Qty ];
	double *stdDevX = new double[ g_pOptParams->Qty ];

	for( i = 0; i < g_pOptParams->Qty; i++ )
	{
		initX[ i ] = (g_pOptParams->Items[ i ].Min + g_pOptParams->Items[ i ].Max )/2; 
		stdDevX[ i ] = (g_pOptParams->Items[ i ].Max - g_pOptParams->Items[ i ].Min )/2; 
	}

	arFunvals = cmaes_init(&evo, g_pOptParams->Qty, initX, stdDevX, 0, lambda, "non"); 

	delete[] initX;
	delete[] stdDevX;

	//cmaes_ReadSignals(&evo, "signals.par");  /* write header and initial values */ 
}

void gran( int N, double *val )
{
	// this is special function that implements
	// "granularity"
	// by design CMA-ES works CONTINUOUS spaces
	// so we have to force min/max/step constraints

	*val =  g_pOptParams->Items[ N ].Step * floor( 0.5 + ( *val / g_pOptParams->Items[ N ].Step ) );

	if( *val > g_pOptParams->Items[ N ].Max	) *val = g_pOptParams->Items[ N ].Max;
	if( *val < g_pOptParams->Items[ N ].Min	) *val = g_pOptParams->Items[ N ].Min;
}

double fitfun(double const *x, int dim); 

/* the objective (fitness) function to be minized */
double fitfun(double const *x, int N) 
{ 
	for( int k = 0; k < N; k++ )
	{
		g_pOptParams->Items[ k ].Current = x[ k ];
	}

	// if we reached "numsteps" already - increase it
	// to continue optimization for more than "estimated" number of steps
    if( g_pOptParams->NumSteps <= g_pOptParams->Step + 1 )
		g_pOptParams->NumSteps = g_pOptParams->Step + 10;
	
	return - g_pfEvaluateFunc( g_pOptimizerContext ); // return with negative sign so max is found
} 

int optimize( void )
{
  int iContinue = ! cmaes_TestForTermination( &evo );

  if( iContinue )
  {
     /* generate lambda new search points, sample population */
      pop = cmaes_SamplePopulation(&evo); /* do not change content of pop */

      /* Here you may resample each solution point pop[i] until it
	 becomes feasible, e.g. for box constraints (variable
	 boundaries). function is_feasible(...) needs to be
	 user-defined.  
	 Assumptions: the feasible domain is convex, the optimum is
	 not on (or very close to) the domain boundary, initialX is
	 feasible and initialStandardDeviations are sufficiently small
	 to prevent quasi-infinite looping.
      */
      /* for (i = 0; i < cmaes_Get(&evo, "popsize"); ++i) 
	   while (!is_feasible(pop[i])) 
	     cmaes_ReSampleSingle(&evo, i); 
      */

      /* evaluate the new search points using fitfun from above */ 
      for (i = 0; i < cmaes_Get(&evo, "lambda"); ++i) 
	  {
		arFunvals[i] = fitfun(pop[i], (int) cmaes_Get(&evo, "dim"));
      }

      /* update the search distribution used for cmaes_SampleDistribution() */
      cmaes_UpdateDistribution(&evo, arFunvals); 
	  

      /* read instructions for printing output or changing termination conditions */ 
      //cmaes_ReadSignals(&evo, "signals.par");     
  }

  if( ! iContinue )
  {
	  run++;

	  // increase lambda in next run
	  // that causes more tests and theoretically
	  // ensures that with each run it is more probable
	  // to find global maximum instead of local.
      lambda = (int) ( incpopsize * cmaes_Get(&evo, "lambda") );   /* needed for the restart */

	  // re-initialize for next run

	  final(); /* release memory */ 
	  init();
	  iContinue = 1;

	  if( run >= runMax ||
		  g_pOptParams->Step > ( runMax * evalMax ) )
	  {
		  iContinue = 0;  // protection	against exceeding runs
	  }

  	  TRACE("Run %d, step %d, NumSteps %d, Cont %d\n", run, (int)g_pOptParams->Step, (int)g_pOptParams->NumSteps, iContinue );
  }

  return iContinue;
}

void final()
{
  //printf("Stop:\n%s\n",  cmaes_TestForTermination(&evo)); /* print termination reason */
  //cmaes_WriteToFile(&evo, "all", "allcmaes.dat");         /* write final results */

  /* get best estimator for the optimum, xmean */
  xfinal = cmaes_GetNew(&evo, "xmean"); /* "xbestever" might be used as well */
  cmaes_exit(&evo); /* release memory */ 

  /* do something with final solution and finally release memory */
  free(xfinal); 
}
