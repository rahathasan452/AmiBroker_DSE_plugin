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
#include "math.h"

#ifndef ASSERT
#include "assert.h"
#define ASSERT assert
#endif

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

struct StockArrays
{
	int	   Size;
	float *Open;
	float *High;
	float *Low;
	float *Close;
	float *Volume;
	AmiVar Result;
	AmiVar AvgRange;
};


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

void GetStockArrays( struct StockArrays *sa )
{
	sa->Size	= gSite.GetArraySize();
	sa->Open	= gSite.GetStockArray( 0 );
	sa->High	= gSite.GetStockArray( 1 );
	sa->Low		= gSite.GetStockArray( 2 );
	sa->Close	= gSite.GetStockArray( 3 );
	sa->Volume	= gSite.GetStockArray( 4 );

	sa->Result	= gSite.AllocArrayResult();
	sa->AvgRange = gSite.AllocArrayResult();
}

#define AVG_RANGE 15

void CalcAverageRange( struct StockArrays *sa )
{
	int i;

	float sum = 0.0f;

	for( i = 0; i < sa->Size; i++ )
	{
		if( i < AVG_RANGE )
		{
			sum += (float) fabs( sa->Close[ i ] - sa->Open[ i ] );

			sa->AvgRange.array[ i ] = EMPTY_VAL;
		}
		else
		{
			sum += (float) ( fabs( sa->Close[ i ] - sa->Open[ i ] ) -  fabs( sa->Close[ i - AVG_RANGE ]  - sa->Open[ i - AVG_RANGE ] ) );
		
			sa->AvgRange.array[ i ] = sum / AVG_RANGE;
		}
	}
}

AmiVar Candle( int NumArgs, AmiVar *ArgsTable, BOOL bBlack )
{
	int i, j;

	ASSERT( NumArgs == 2 );

	struct StockArrays sa;

	GetStockArrays( &sa );

	CalcAverageRange( &sa );

	j = SkipEmptyValues( sa.Size, sa.AvgRange.array, sa.Result.array );

	float fBodyFactor = ArgsTable[ 0 ].val;
	float fRangeFactor = ArgsTable[ 1 ].val;

	for( i = j; i < sa.Size; i++ )
	{
		float fBody = sa.Close[ i ] - sa.Open[ i ];
		float fCandle = sa.High[ i ] - sa.Low[ i ];

		if( bBlack ) fBody = -fBody; 

		sa.Result.array[ i ] = fBody > 0 &&
			                   fBody >= fBodyFactor * fCandle &&
							   fBody >= fRangeFactor * sa.AvgRange.array[ i ];
	}

	return sa.Result;
}

// CdWhite
AmiVar VCdWhite( int NumArgs, AmiVar *ArgsTable )
{
	return Candle( NumArgs, ArgsTable, FALSE );
}

// CdBlack
AmiVar VCdBlack( int NumArgs, AmiVar *ArgsTable )
{
	return Candle( NumArgs, ArgsTable, TRUE);
}

// CdDoji
AmiVar VCdDoji( int NumArgs, AmiVar *ArgsTable )
{
	ASSERT( NumArgs == 1 );

	struct StockArrays sa;

	GetStockArrays( &sa );

	for( int i = 0; i < sa.Size; i++ )
	{
		sa.Result.array[ i ] = fabs( sa.Close[ i ] - sa.Open[ i ] ) <= ArgsTable[ 0 ].val * ( sa.High[ i ] - sa.Low[ i ] );
	}

	return sa.Result;
}

// CdHammer
AmiVar VCdHammer( int NumArgs, AmiVar *ArgsTable )
{
	int i, j;
	
	ASSERT( NumArgs == 1 );

	struct StockArrays sa;

	GetStockArrays( &sa ); 

	CalcAverageRange( &sa );

	j = SkipEmptyValues( sa.Size, sa.AvgRange.array, sa.Result.array );

	float fHeightFactor = ArgsTable[ 0 ].val;

	for( i = j; i < sa.Size; i++ )
	{
		float fCandle = sa.High[ i ] - sa.Low[ i ];
		sa.Result.array[ i ] = fCandle > fHeightFactor * sa.AvgRange.array[ i ] &&
							   sa.Close[ i ] > 0.5f * ( sa.High[ i ] + sa.Low[ i ] ) && 
							   sa.Open[ i ] > sa.Close[ i ] /* black ! */  &&
							   sa.High[ i ] - sa.Open[ i ] < 0.1 * fCandle;
	}

	return sa.Result;
}


AmiVar Engulfing( int NumArgs, AmiVar *ArgsTable, BOOL bBullish )
{
	int i, j;

	ASSERT( NumArgs == 2 );

	struct StockArrays sa;

	GetStockArrays( &sa );

	CalcAverageRange( &sa );

	j = SkipEmptyValues( sa.Size, sa.AvgRange.array, sa.Result.array );

	float fBodyFactor = ArgsTable[ 0 ].val;
	float fRangeFactor = ArgsTable[ 1 ].val;

	BOOL bBlack, bWhite, bPrevBlack, bPrevWhite;

	bPrevBlack = bPrevWhite = FALSE;

	BOOL bEngulf = FALSE;

	for( i = j; i < sa.Size; i++ )
	{
		float fBody =  sa.Close[ i ] - sa.Open[ i ];
		float fCandle = sa.High[ i ] - sa.Low[ i ];
		
		bBlack				 = ( -fBody ) > 0 &&
			                   ( -fBody ) >= fBodyFactor * fCandle &&
							   ( -fBody ) >= fRangeFactor * sa.AvgRange.array[ i ];

		bWhite				 = ( fBody ) > 0 &&
			                   ( fBody ) >= fBodyFactor * fCandle &&
							   ( fBody ) >= fRangeFactor * sa.AvgRange.array[ i ];


		if( bBullish )
		{
			bEngulf =	bWhite && bPrevBlack && 
						sa.Close[ i ] > sa.Open[ i - 1 ] &&
						sa.Open[ i ] < sa.Close[ i - 1 ];
		}
		else
		{
			bEngulf =	bBlack && bPrevWhite && 
						sa.Close[ i ] < sa.Open[ i - 1 ] &&
						sa.Open[ i ] > sa.Close[ i - 1 ];
		}

		bPrevBlack = bBlack;
		bPrevWhite = bWhite;

		sa.Result.array[ i ] = (float) bEngulf;
	}

	return sa.Result;
}

AmiVar VCdBullEngulfing( int NumArgs, AmiVar *ArgsTable )
{
	return Engulfing( NumArgs, ArgsTable, TRUE );
}

AmiVar VCdBearEngulfing( int NumArgs, AmiVar *ArgsTable )
{
	return Engulfing( NumArgs, ArgsTable, FALSE );
}

//////////////////////////////////
// Tables of default values
// for functions
float candle_def[] = { 0.4f, 0.0f };
float big_def[] = { 0.9f, 1.5f };
float doji_def[] = { 0.05f };	// 5% threshold for detecting doji
float hammer_def[] = { 1.1f };	 
float engulf_def[] = { 0.4f, 0.5f };

/////////////////////////////////////////////
// Function table now follows
//
// You have to specify each function that should be
// visible for AmiBroker.
// Each entry of the table must contain:
// "Function name", { FunctionPtr, <no. of array args>, <no. of string args>, <no. of float args>, <no. of default args>, <pointer to default values table float *>

FunctionTag gFunctionTable[] = {
								"CdWhite",				{ VCdWhite, 0, 0, 0, 2, candle_def }, 
								"CdBlack",				{ VCdBlack, 0, 0, 0, 2, candle_def }, 
								"CdBigWhite",			{ VCdWhite, 0, 0, 0, 2, big_def }, 
								"CdBigBlack",			{ VCdBlack, 0, 0, 0, 2, big_def }, 
								"CdDoji",				{ VCdDoji, 0, 0, 0, 1,  doji_def }, 
								"CdHammer",				{ VCdHammer, 0, 0, 0, 1,  hammer_def }, 
								"CdBearishEngulfing",	{ VCdBearEngulfing, 0, 0, 0, 2,  engulf_def }, 
								"CdBullishEngulfing",	{ VCdBullEngulfing, 0, 0, 0, 2,  engulf_def }
};

int gFunctionTableSize = sizeof( gFunctionTable )/sizeof( FunctionTag );
