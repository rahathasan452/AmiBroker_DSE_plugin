////////////////////////////////////////////////////
// Functions.cpp
// Sample functions implementation file for example AmiBroker plug-in
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


///////////////////////////////////////////
// Each AFL function has the following prototype:
//
// AmiVar FunctionName( int NumArgs, AmiVar *ArgsTable )
//
// You can define as many functions as you want.
// To make them visible you should add them to the function
// table at the bottom of this file
//
///////////////////////////////////////////////


// ExampleMACD() 
// This function demonstrates
// how to call-back internal AFL function
AmiVar VExampleMACD( int NumArgs, AmiVar *ArgsTable )
{
	// First you need to fill-in
	// arguments table.
	//
	// we will call back MACD that gets two float
	// arguments:
	
	AmiVar arg[ 2 ];
		
	arg[ 0 ].type = VAR_FLOAT;
	arg[ 0 ].val = 12;

	arg[ 1 ].type = VAR_FLOAT;
	arg[ 1 ].val = 26;

	
	// Now you can call internal AFL function
	// using CallFunction callback pointer of the site interface
	// Please note that NO ERROR checking is done
	// on number/types of arguments passed
	// Specifying wrong args here ends with access violation in most cases
    AmiVar result = gSite.CallFunction( "macd", 2, arg );

	return result;
}


// ExampleMA( array, periods )
// This function demonstrates
// how to calculate 
// simple moving average

AmiVar VExampleMA( int NumArgs, AmiVar *ArgsTable )
{
    int i, j, k;
	float sum;
    AmiVar result;

    result = gSite.AllocArrayResult();

	int nSize = gSite.GetArraySize();

	float *SrcArray = ArgsTable[ 0 ].array;
	int nRange = (int) ArgsTable[ 1 ].val;

	j = SkipEmptyValues( nSize, SrcArray, result.array );

    for( i =  j; i < nSize; i++ )
    {
		////////////////////////////
		// check if there is enough data to calculate average
		// skip elsewhere
		////////////////////////////
		if( i < j + nRange ) 
		{
			result.array[ i ] = EMPTY_VAL;
			continue;
		}

		////////////////////////////////
		// calculate sum of <nRange> recent values
		////////////////////////////////
		sum = 0;

		for( k = 0; k < nRange; k++ )
		{
			sum += SrcArray[ i - k ];
		}

		/////////////////////////////////
		// store the average
		/////////////////////////////////
        result.array[ i ] = (float) sum/nRange;
    }
    return result;
}


// ExampleEMA( array, periods )
// This function demonstrates
// how to calculate 
// exponential moving average

AmiVar VExampleEMA( int NumArgs, AmiVar *ArgsTable )
{
    int i, j;
    AmiVar result;

    result = gSite.AllocArrayResult();

	int nSize = gSite.GetArraySize();

	float *SrcArray = ArgsTable[ 0 ].array;

	j = SkipEmptyValues( nSize, SrcArray, result.array );


	// exponential mov. avg. factor is 2/(Range+1)
	float factor = 2.0f/( ArgsTable[ 1 ].val + 1 );

	float average = EMPTY_VAL;

	if( j < nSize ) average = SrcArray[ j ];

    for( i = j + 1; i < nSize; i++ )
    {
		///////////////////////////////////////////////
		// the line below calculates current EMA value
		///////////////////////////////////////////////
		average = factor * SrcArray[ j ] + ( 1.0f - factor ) * average;

		// store the result
        result.array[ i ] = average;
    }
    return result;
}


/////////////////////////////////////////////
// Function table now follows
//
// You have to specify each function that should be
// visible for AmiBroker.
// Each entry of the table must contain:
// "Function name", { FunctionPtr, <no. of array args>, <no. of string args>, <no. of float args>, <no. of default args>, <pointer to default values table float *>

FunctionTag gFunctionTable[] = {
								"ExampleMACD",		{ VExampleMACD, 0, 0, 0, 0, NULL }, 
								"ExampleMA",        { VExampleMA, 1, 0, 1, 0, NULL }, 
								"ExampleEMA",        { VExampleMA, 1, 0, 1, 0, NULL } 
};

int gFunctionTableSize = sizeof( gFunctionTable )/sizeof( FunctionTag );
