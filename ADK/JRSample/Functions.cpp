////////////////////////////////////////////////////
// Functions.cpp
// Sample functions implementation file for example AmiBroker plug-in
//
// Copyright (C)2001 Tomasz Janeczko, amibroker.com
// All rights reserved.
//
// Last modified: 2001-12-17 TJ
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

#pragma comment( lib, "jrs_32.lib" )

extern "C" __declspec(dllimport) int WINAPI JMA( int iLength, double *pdSignal, double dSpeed, double dPhase, double *pdFilter );

// Helper function

int SkipEmptyValues( int nSize, float *Src, float *Dst )
{
	int i;

	for( i = 0; i < nSize && IS_EMPTY( Src[ i ] ); i++ )
	{
		Dst[ i ] = EMPTY_VAL;
	}

	return i;
}



AmiVar VJurikMA( int NumArgs, AmiVar *ArgsTable )
{
    int i, j;
    AmiVar result;

    result = gSite.AllocArrayResult();

	int nSize = gSite.GetArraySize();

	float *SrcArray = ArgsTable[ 0 ].array;
	double dSmooth =  ArgsTable[ 1 ].val;
	double dPhase =  ArgsTable[ 2 ].val;

	j = SkipEmptyValues( nSize, SrcArray, result.array );

	if( j < nSize )
	{
		// calculate only non-empty values

		///////////////////////////////////////
		// Jurik MA works on arrays of doubles
		// so we need to convert from floats
		// to double before calling JMA..........
		////////////////////////////////////////
		double *pdSignal = new double[ nSize - j ];
		double *pdFiltered = new double[ nSize - j ];

		for( i =  j; i < nSize; i++ )
		{
			pdSignal[ i - j ] = SrcArray[ i ];			
		}


		////////////////////////////////////////////
		// Now we call JMA from the DLL
		////////////////////////////////////////////
		JMA( nSize - j, pdSignal, dSmooth, dPhase, pdFiltered );

		///////////////////////////////////////////
		// .... and convert the result array 
		// of doubles to floats after JMA call
		///////////////////////////////////////////
		
		for( i =  j; i < nSize; i++ )
		{
			result.array[ i ] = (float) pdFiltered[ i - j ];			
		}

		////////////////////////////////////////////
		// Release temporary tables
		////////////////////////////////////////////
		delete[] pdSignal;
		delete[] pdFiltered;
	}
    return result;
}


float jma_def[] = { 10, 0 };

/////////////////////////////////////////////
// Function table now follows
//
// You have to specify each function that should be
// visible for AmiBroker.
// Each entry of the table must contain:
// "Function name", { FunctionPtr, <no. of array args>, <no. of string args>, <no. of float args>, <no. of default args>, <pointer to default values table float *>

FunctionTag gFunctionTable[] = {
								"JurikMA",        { VJurikMA, 1, 0, 0, 2, jma_def }, 
};

int gFunctionTableSize = sizeof( gFunctionTable )/sizeof( FunctionTag );
