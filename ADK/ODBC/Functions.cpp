////////////////////////////////////////////////////
// Functions.cpp
// Sample functions implementation file for example AmiBroker plug-in
//
// Copyright (C)2001-2006 Tomasz Janeczko, amibroker.com
// All rights reserved.
//
// Last modified: 2006-07-21 TJ
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
#include "Global.h"
#include "SRecordset.h"

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


// odbcOpenDatabase() 
AmiVar VOpenDatabase( int NumArgs, AmiVar *ArgsTable )
{
	g_oData.ConnectAFL( ArgsTable[ 0 ].string );

	AmiVar result;
	result.val = (float) g_oData.ConnectAFL( ArgsTable[ 0 ].string );
	result.type = VAR_FLOAT;

	return result;
}

// odbcSetFieldNames( "symbolfield", "datefield" )
AmiVar VSetFieldNames( int NumArgs, AmiVar *ArgsTable )
{
	g_oData.m_oAflSymbolField = ArgsTable[ 0 ].string;
	g_oData.m_oAflDateField = ArgsTable[ 1 ].string;

	AmiVar result;
	result.val = 1;
	result.type = VAR_FLOAT;

	return result;

}

// This assumes that 

// odbcGetArray( "tablename", "symbol", "field" )

// This assumes that 

// odbcGetArray( "tablename", "symbol", "field" )


AmiVar GetArrayHelper( const CString& oQuery )
{
    AmiVar result;

    result = gSite.AllocArrayResult();

	int nSize = gSite.GetArraySize();

	if( gSite.nStructSize < sizeof( SiteInterface ) )
	{
		g_oData.m_oLastErrorMsg = "Unsupported, old AmiBroker version. You need to upgrade to AmiBroker 5.30 or higher";
		return result;
	}

	// retrieve datetime array
	AmiDate *datearray = (AmiDate *) gSite.GetDateTimeArray();

	int iRefBar = 0;
	float lastvalue = EMPTY_VAL;

	try
	{
		CDatabase *poDB = g_oData.GetAflDatabase();

		if( poDB )
		{
			CSRecordset oSet( poDB );

			int iNumRows = 0;

			unsigned int lastdatetime = 0;

			if( oSet.Open( CRecordset::forwardOnly,
							oQuery ) )
			{
				while( !oSet.IsEOF() )
				{
					CDBVariant oVar;

					oSet.GetFieldValue( (short) 0, oVar, SQL_C_FLOAT );

					float value = oVar.m_fltVal;

					oSet.GetFieldValue( (short) 1, oVar, SQL_C_TIMESTAMP );

					AmiDate datevalue;

					if( oVar.m_pdate->hour == 0 &&
						oVar.m_pdate->minute == 0 &&
						oVar.m_pdate->second == 0 )
					{
						datevalue.Date = DAILY_MASK;
					}
					else
					{
						datevalue.Date = 0;
						datevalue.PackDate.Hour = oVar.m_pdate->hour;
						datevalue.PackDate.Minute = oVar.m_pdate->minute;
						datevalue.PackDate.Second = oVar.m_pdate->second;
					}

 					datevalue.PackDate.Year = oVar.m_pdate->year;
					datevalue.PackDate.Month = oVar.m_pdate->month;
					datevalue.PackDate.Day = oVar.m_pdate->day;


					if( lastdatetime > datevalue.Date )
					{
						g_oData.m_oLastErrorMsg = "Invalid order of records in a query. Data should be sorted in ASCENDING date/time order, use 'ORDER BY Date ASC' clause in your SQL query to sort correctly";
						break;
					}

					lastdatetime = datevalue.Date;

					// search for bar that is equal or greater than 
					// the bar read from the file
					while( iRefBar < nSize && datearray[ iRefBar ].Date < datevalue.Date )
					{
						result.array[ iRefBar++ ] = lastvalue;
					}

					if( iRefBar >= nSize ) break;

					result.array[ iRefBar ] = lastvalue = value;

					oSet.MoveNext();
				}
			}
		}
	}
	catch( CDBException *e )
	{
		g_oData.StoreErrorMessage( e );

		e->Delete();

		AmiVar fault;
		fault.type = VAR_FLOAT;
		fault.val = EMPTY_VAL;

		return fault;
	}

	for( ; iRefBar < nSize; iRefBar++ )
	{
		result.array[ iRefBar ] = lastvalue;
	}

    return result;
}


AmiVar VGetArray( int NumArgs, AmiVar *ArgsTable )
{
	CString oTable = ArgsTable[ 0 ].string;
	CString oSymbol = ArgsTable[ 1 ].string;
	CString oField = ArgsTable[ 2 ].string;

	AmiVar result; 

	if( oSymbol.GetLength() == 0 )
	{
		AmiVar ticker = gSite.CallFunction( "name", 0, &result );

		oSymbol = ticker.string;
	}

	CString oQuery;

	oQuery.Format("SELECT %s, %s FROM %s WHERE %s='%s' ORDER BY %s ASC",
				   g_oData.Wrap( oField ),
				   g_oData.Wrap( g_oData.m_oAflDateField ),
				   g_oData.Wrap( oTable ),
				   g_oData.Wrap( g_oData.m_oAflSymbolField ),
				   oSymbol,
				   g_oData.Wrap( g_oData.m_oAflDateField ) );


	return GetArrayHelper( oQuery );
}

// This assumes that 

// odbcGetArray( "tablename", "symbol", "field" )


AmiVar VGetArraySQL( int NumArgs, AmiVar *ArgsTable )
{
	CString oQuery = ArgsTable[ 0 ].string;

	return GetArrayHelper( oQuery );
}

// odbcGetValue( "tablename", "symbol", "field" )


AmiVar GetValueHelper( const CString &oQuery )
{
    AmiVar result;

	result.type = VAR_FLOAT;
	result.val = EMPTY_VAL;

	try
	{
		CDatabase *poDB = g_oData.GetAflDatabase();

		if( poDB )
		{
			CSRecordset oSet( poDB );

			int iNumRows = 0;

			if( oSet.Open( CRecordset::forwardOnly,
							oQuery ) )
			{
				if( !oSet.IsEOF() )
				{
					CString oValStr;

					oSet.GetFieldValue( (short)0, oValStr );

					char *endptr = NULL;

					// attempt to convert to number
					double dValue = strtod(	oValStr, &endptr );

					if( endptr == NULL || *endptr == '\0' )
					{
						// conversion went fine

						result.val = (float) dValue;
					}
					else
					{
						// conversion failed
						// return STRING value instead

						result.string = (char *)gSite.Alloc( oValStr.GetLength() + 1 );
						strcpy( result.string, oValStr );
						result.type = VAR_STRING;
					}

				}
			}
		}
	}
	catch( CDBException *e )
	{
		g_oData.StoreErrorMessage( e );

		e->Delete();

		AmiVar fault;
		fault.type = VAR_FLOAT;
		fault.val = EMPTY_VAL;

		return fault;
	}


    return result;
}

AmiVar VGetValue( int NumArgs, AmiVar *ArgsTable )
{
	CString oTable = ArgsTable[ 0 ].string;
	CString oSymbol = ArgsTable[ 1 ].string;
	CString oField = ArgsTable[ 2 ].string;

	AmiVar result;

	if( oSymbol.GetLength() == 0 )
	{
		AmiVar ticker = gSite.CallFunction( "name", 0, &result );

		oSymbol = ticker.string;
	}

	CString oQuery;

	oQuery.Format("SELECT %s FROM %s WHERE %s='%s'",
				   g_oData.Wrap( oField ),
				   g_oData.Wrap( oTable ),
				   g_oData.Wrap( g_oData.m_oAflSymbolField ), 
				   oSymbol );

	return GetValueHelper( oQuery );
}

AmiVar VGetValueSQL( int NumArgs, AmiVar *ArgsTable )
{
	CString oQuery = ArgsTable[ 0 ].string;

	return GetValueHelper( oQuery );
}


AmiVar VExecuteSQL( int NumArgs, AmiVar *ArgsTable )
{
    AmiVar result;

	result.type = VAR_FLOAT;
	result.val = 0;


	CString oSQL = ArgsTable[ 0 ].string;

	try
	{
		CDatabase *poDB = g_oData.GetAflDatabase();

		if( poDB )
		{
			poDB->ExecuteSQL( oSQL );
			result.val = 1;
		}
	}
	catch( CDBException *e )
	{
		g_oData.StoreErrorMessage( e );

		e->Delete();

		AmiVar fault;
		fault.type = VAR_FLOAT;
		fault.val = 0;

		return fault;
	}


    return result;
}


AmiVar VGetLastError( int NumArgs, AmiVar *ArgsTable )
{
    AmiVar result;

	result.type = VAR_STRING;
	result.string = (char *)gSite.Alloc( g_oData.m_oLastErrorMsg.GetLength() + 1 );

	strcpy( result.string, g_oData.m_oLastErrorMsg ); 

	return result;

}

AmiVar VDisplayErrors( int NumArgs, AmiVar *ArgsTable )
{
    AmiVar result;

	result.type = VAR_FLOAT;
	result.val = (float) g_oData.m_bDisplayErrors;

	g_oData.m_bDisplayErrors = (BOOL) ArgsTable[ 0 ].val;

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
								"odbcOpenDatabase( connection )",		{ VOpenDatabase, 0, 1, 0, 0, NULL }, 
								"odbcSetFieldNames( symbolfield, datefield )",		{ VSetFieldNames, 0, 2, 0, 0, NULL }, 
								"odbcGetArray( table, symbol, field )",		{ VGetArray, 0, 3, 0, 0, NULL }, 
								"odbcGetArraySQL( \"sql statement\" )",		{ VGetArraySQL, 0, 1, 0, 0, NULL }, 
								"odbcGetValue( table, symbol, field )",		{ VGetValue, 0, 3, 0, 0, NULL }, 
								"odbcGetValueSQL( \"sql statement\" )",		{ VGetValueSQL, 0, 1, 0, 0, NULL }, 
								"odbcExecuteSQL( \"sql statement\" )",		{ VExecuteSQL, 0, 1, 0, 0, NULL }, 
								"odbcGetLastError()",		{ VGetLastError, 0, 0, 0, 0, NULL }, 
								"odbcDisplayErrors( enable )",		{ VDisplayErrors, 0, 0, 1, 0, NULL }, 
};

int gFunctionTableSize = sizeof( gFunctionTable )/sizeof( FunctionTag );
