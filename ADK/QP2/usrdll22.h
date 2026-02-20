/**************************************************************************
* File: usrdll22.h
* Purpose: Declaration For Quotes Plus Dll Functionality
***************************************************************************
* Author: Quotes Plus
**************************************************************************/
#ifndef USRDLL22_H
#define USRDLL22_H
#pragma pack( push, _USRDLL11_H_, 1 )
//================Begin Quotes Plus Error Defines =========================
#define QPERROR_NOT_INITIALIZED		-20	/*Dll Not Initialized: 
											Call R2_InitReadstock*/
#define QPERROR_CTREEINIT_FAILED	-21	/*C-TREE Dll Initialization Failed
											for current thread*/
#define QPERROR_ALREADY_INITIALIZED	-22	/*R2_InitReadStock has already
											been called for the current
											thread*/
#define QPERROR_SYSTEM_TABLE		-23	/*Error loading the system 
											table(system.qpf) failed*/
#define QPERROR_CDREQ_DATE_FILE		-24	/*Error loading CD Required Date
											file*/
#define QPERROR_PRICE_KEY_FILE		-25	/*Error loading Price Key File
											(PriceKeys)*/
#define QPERROR_CT_INDEX_FILE		-26	/*Error loading C-TREE Index 
											File(ctindex.idx)*/
#define QPERROR_FNDMTL_DB_FILE		-27	/*Error loading Fundamental 
											Database File(master.qpf)*/
#define QPERROR_FILE				-28	/*Error loading/opening/closing
											file requested*/
#define QPERROR_FILE_INVALID		-29	/*Invalid file requested*/
#define QPERROR_NOTOPEN				-30	/*Data File Not Open*/

#define QPERROR_IDXINVALID			-31	/*The index used is not a valid
											index in the file*/
#define QPERROR_NAVINVALID			-32	/*The navigation value used is not
											a valid navigation operator
											for the file*/
//===============End Quotes Plus Error Defines=============================
//=================Begin Quotes Plus File Identifiers======================
/*NOTE: These Definitions are used in the R2_Open_Files and R2_Close_Files
			Functions*/
#define DAILY_FILES         1	/*Opens all of the daily files:
									All Price Files, SPLIT_FILE*/
#define WEEKLY_FILES        2	/*Opens the weekly price files as well as
									the SPLIT_FILE*/
#define MASTER_FILE         3	/*Opens the fundamental file for stocks and
									indexes*/
#define NAME_CHG_FILE       4   /*Opens the Name Change File */
#define SPLIT_FILE          5	/*Opens the Split File*/
#define SCANDATA_FILE		7	/*Opens the Scan Database File*/
#define FUND_DATA			10	/*Opens the Mutual Fund Price File*/
#define FUND_MASTER			11	/*Opens the Mutual Fund fundamentals file*/
#define FUTURE_MASTER		12	/*Opens the Commodity Fundamentals file*/
#define FUND_DISTRIBUTIONS	13	/*Opens the Mutual Fund Distributions 
									File*/
#define IRL_DATA			14	/*Opens the IRL Industry group data file*/
#define SIC_DATA			15	/*Opens the SIC code data file*/
#define ALL_DATA_FILES		17	/*Opens ALL OF THE DATA FILES:
									DAILY_FILES, FUND_DATA, WEEKLY_FILES,
									FUTURE_DATA*/
#define FUTURE_DATA 		18	/*Opens the Commodity Data File*/
#define DIVIDEND_TABLE		31	/*Opens the Stock Dividends Table*/
//------------------Begin Zacks Data Tables--------------------------------
#define ANALYSTREC_TABLE	33	/*Opens the Analyst Recommendations Table*/
#define EEC_TABLE			34	/*Opens the Earnings stimate Consensus 
									Table*/
#define EXPEARN_TABLE		35	/*Opens the Expected Earnings Table*/
#define EARNEST_TABLE		36  /*Opens the Earnings Estimate Table*/
#define ZACKS_DATA			38	/*Opens All of the Zacks Tables: 
									ANALYSTREC_TABLE, EEC_TABLE,
									EXPEARN_TABLE, EARNEST_TABLE,
									ZACKS_DATA_TABLE, ZACKS_EPS_TABLE*/
#define ZACKS_DATA_TABLE	39	/*Opens the zacks fundamental information 
									table*/								
#define ZACKS_EPS_TABLE		40	/*Opens the zacks EPS data Table*/
//-------------------End Zacks Data Tables---------------------------------
#define IDXCOMP_TABLE		37	/*Opens the Index Components Table*/
#define EPS_RANK_TABLE		41	/*Opens the Eps Rank Table*/
//==================End Quotes Plus File Identifiers=======================
//===============Begin Quotes Plus File Navigation Identifiers=============
/*NOTE: Used in calls to:*/
#define QP_EQUAL   0	//Retrieve record equal to key
#define QP_APPROX  1	//Retrieve record greater than or equal to key
#define QP_NEXT    2	//Retrieve next record
#define QP_PREV    3	//Retrieve previous record
#define QP_TOP     4	//Retrieve first record in database/file
#define QP_BOTTOM  5	//Retrieve last record in database/file
#define QP_CURRENT 6	//Retrieve current record
//=================End Quotes Plus File Navigation Identifiers=============
//==================Begin Quotes Plus Index Identifiers====================
#define QP_SYMBOL		0		//Ticker symbol
#define QP_NAME			1		//Issue Description + Issuer Description
#define QP_OLDSYMBOL	3       //Only used in the R2_QP_ReadNameChg functon
#define QP_NEWSYMBOL	4		//Only used in the R2_QP_ReadNameChg functon
#define QP_CUSIP		5		//Cusip Identifier
#define QP_SECTOR		7		//Industry Sector
#define QP_YEAR			9		//Year
#define QP_SIC			14		//SIC Code
#define QP_MSX			15      //IRL Industry Group/Sector Symbol
#define QP_GROUPID		16		//Industry Group Identifier
#define QP_INDEX		17		//Component Index
#define QP_COMPONENT	18		//Component Index + Symbol
#define QP_SYMEXDATE	19		//Symbol + Expiration Date
#define QP_LONGSYM		20		//Long Symbol - Used For Futures
#define QP_INDUSTRY		21		//Industry Group
#define QP_EPSRANK		22		//Eps Ranking
//====================End Quotes Plus Index Identifiers====================
//===================Begin Quotes Plus Date Identifiers====================
#define DATE_LOCAL          1 
#define DATE_CD             4  
#define DATE_ALL            7 
//======================End Quotes Plus Date Identifiers===================
//******************Begin Quotes Plus Data Records*************************
//===================Begin Declaration For Stock_Record====================
/*NOTE: This Record is used in the calls to: R2_QP_LoadSymbol,
			R2_QP_LoadWkSymbol*/
typedef struct 
{
	char MM;				//Month
	char DD;				//Day of Month
	short YY;				//Year
	float open;				//Open Price
	float hi;				//High Price
	float lo;				//Low Price
	float cl;				//Closing Price
	long  vol;				//Volume
	char RS;				//Quotes Plus Relative Strength			
	float oi;				//Open interest
} *LPSTOCK_RECORD, Stock_Record;
//==================End Declaration For Stock_Record=======================
//===============Begin Master Record Table Ticker Type Enumeration=========
/*This is used to determine if the ticker is a stock, mutual fund, 
	commodity, etc.  It is not used to determine what exchange the ticker
	is in.*/
typedef enum QP_TICKER_TYPES
{
	qp_ticker_invalid,	//Invalid Ticker Type
	qp_ticker_stock,	//Stock 
	qp_ticker_fund,		//Mutual Fund
	qp_ticker_contract,	//Commodity - Contract
	qp_ticker_cash,		//Commodity - Cash
	qp_ticker_cont		//Commodity - Continuous
} QP_TICKER_TYPE;
//=================End Master Record Table Ticker Type Enumeration=========
//==================Begin Declaration For Master_Rec=======================
//Note: Stocks and Funds will use the same record
typedef struct  
{
	char exchange[2];			//Exchange Code
	char exchange_sub;			//Exchange Sub Code
	char CUSIP[8];				//CUSIP Identifier
	char symbol[6];				//Ticker Symbol
	char issuer_desc[28];		//Issuer Description
	char issue_desc[15];		//Issue Description
	char issue_type;			//Issue Type Code
	char issue_status;			//Issue Status Code
	char SIC[4];				//SIC Code
	float IAD;					//Annual Dividend
	char IAD_footnote;			//Annual Dividend Footnote
	float EPS;					//Earnings Per Share
	char xref_CUSIP[9];			//Cross Reference CUSIP
	char EPS_footnote;			//Earnings Per Share Footnote
	char EPS_end_mn;			//Earnings Per Share End Month
	char EPS_end_yr;			//Earnings Per Share End Year
	long last_main_date;		//Last Maintenance Date YYYYMMDD
	long shares;				//Shares Outstanding
	char margin_flag;			//Margin Flag(Option Flag)
	char flags;					//Record Flags
	QP_TICKER_TYPE TickerType;	//Ticker Type, Refer to enumeration QP_TICKER_TYPES

} *LPMASTER_REC, Master_Rec;
//====================End Declaration For Master_Rec=======================
//==================Begin Declaration For SplitRec=========================
typedef struct  
{
	char symbol[6];				//Ticker Symbol
	short yy;					//Year
	char mm;					//Month
	char dd;					//Day of Month
	float newquan;				//New Quantity
	float oldquan;				//Old Quantity
	//Split Ratio is newquan/oldquan
} *LPSPLIT_REC, SplitRec;
//===================End Declaration For SplitRec==========================
//================Begin Declaration For QP_DISTRIB_REC=====================
typedef struct 
{
	char Symbol[6];		//Ticker Symbol
	char Flag[2];		//Dividend Flag

	float Amount;		//Dividend Amount

	long AnnDate;		//Announcement Date
	long PayDate;		//Payable Date
	long ExDate;		//Expiration Date
	long RecDate;		//Record Date

} *LPQP_DISTRIB_REC, QP_DISTRIB_REC;
//=================End Declaration For QP_DISTRIB_REC======================
//===================Begin Declaration For QP_IRL_REC======================
typedef struct 
{
	short GroupID;			//Numeric Identifier for the Industry Group

	char Sector[50];		//Sector
	char Industry[50];		//Industry
	char Group[8];			//IRL Symbol or Industry Group

} *LPQP_IRL_REC, QP_IRL_REC;
//=================End Declaration For QP_IRL_REC==========================
//==================Begin Declaration For Sic_Rec==========================
typedef struct 
{
	char sicCode[4];		//SIC Code
	char desc[60];			//SIC Code Description
} *LPSIC_REC, Sic_Rec;
//====================End Declaration For Sic_Rec==========================
//====================Begin Declaration For QP_DATE========================
typedef struct
{
	char month;
	char day;
	__int16  year;
} *LPQP_DATE, QP_DATE;
//=====================End Declaration For QP_DATE=========================
//==================Begin Declaration For Dividend_Rec=====================
typedef struct 
{
	char flag[2];			//Dividend Flag
	
	float rate;				//Amount
	
	QP_DATE Ex_date;		//Expirations Date
	QP_DATE Rec_date;		//Record Date
	QP_DATE Pay_date;		//Payable Date
	QP_DATE Ann_date;		//Announcement Date
	
	char  revision_flag;	//Revision Flag

} *LPDIVIDEND_REC, Dividend_Rec;
//=====================End Declaration For Dividend_Rec====================
//===============Begin Declaration For QP_EARNINGS_EST_CON=================
typedef struct 
{
	double qr1Mean7Day;		//Quarter 1 7 Day Mean
	double qr1Mean30Day;	//Quarter 1 30 Day Mean
	double qr1Mean60Day;	//Quarter 1 60 Day Mean
	double qr1Mean90Day;	//Quarter 1 90 Day Mean

	double qr2Mean7Day;		//Quarter 2 7 Day Mean
	double qr2Mean30Day;	//Quarter 2 30 Day Mean
	double qr2Mean60Day;	//Quarter 2 60 Day Mean
	double qr2Mean90Day;	//Quarter 2 90 Day Mean

	double fr1Mean7Day;		//Fiscal Year 1 7 Day Mean
	double fr1Mean30Day;	//Fiscal Year 1 30 Day Mean
	double fr1Mean60Day;	//Fiscal Year 1 60 Day Mean
	double fr1Mean90Day;	//Fiscal Year 1 90 Day Mean

	double fr2Mean7Day;		//Fiscal Year 2 7 Day Mean
	double fr2Mean30Day;	//Fiscal Year 2 30 Day Mean
	double fr2Mean60Day;	//Fiscal Year 2 60 Day Mean
	double fr2Mean90Day;	//Fiscal Year 2 90 Day Mean

	double LTGMeanCurr;		//Long Term Growth Current Mean
	double LTGMean7Day;		//Long Term Growth 7 Day Mean
	double LTGMean30Day;	//Long Term Growth 30 Day Mean
	double LTGMean60Day;	//Long Term Growth 60 Day Mean
	double LTGMean90Day;	//Long Term Growth 90 Day Mean

} *LPQP_EARNINGS_EST_CON, QP_EARNINGS_EST_CON;
//===============Begin Declaration For QP_EARNINGS_EST_CON=================
//===============Begin Declaration For QP_ANALYST_RECOMMENDATION===========
typedef struct 
{
	short NumStrngBuyCurr;	//Number of Strong Buys - Current
	short NumStrngBuy1Mon;	//Number of Strong Buys 1 Month Ago
	short NumStrngBuy2Mon;	//Number of Strong Buys 2 Months Ago
	short NumStrngBuy3Mon;	//Number of Strong Buys 3 Months Ago

	short NumModBuyCurr;	//Number of Strong Buys 3 Months Ago
	short NumModBuy1Mon;	//Number of Moderate Buys 1 Month Ago
	short NumModBuy2Mon;	//Number of Moderate Buys 2 Months Ago
	short NumModBuy3Mon;	//Number of Moderate Buys 3 Months Ago
	
	short NumHoldsCurr;		//Number of Holds - Current
	short NumHolds1Mon;		//Number of Holds 1 Month Ago
	short NumHolds2Mon;		//Number of Holds 2 Months Ago
	short NumHolds3Mon;		//Number of Holds 3 Months Ago

	short NumModSellCurr;	//Number of Moderate Sells - Current
	short NumModSell1Mon;	//Number of Moderate Sells 1 Month Ago
	short NumModSell2Mon;	//Number of Moderate Sells 2 Months Ago
	short NumModSell3Mon;	//Number of Moderate Sells 3 Months Ago

	short NumStrngSellCurr;	//Number of Strong Sells - Current
	short NumStrngSell1Mon;	//Number of Strong Sells 1 Month Ago
	short NumStrngSell2Mon;	//Number of Strong Sells 2 Months Ago
	short NumStrngSell3Mon;	//Number of Strong Sells 3 Months Ago

	float MeanRecCurr;		//Mean Recommendation - Current
	float MeanRec1Mon;		//Mean Recommendation 1 Month Ago
	float MeanRec2Mon;		//Mean Recommendation 2 Months Ago
	float MeanRec3Mon;		//Mean Recommendation 3 Months Ago

} *LPQP_ANALYST_RECOMMENDATION, QP_ANALYST_RECOMMENDATION;
//===============Begin Declaration For QP_ANALYST_RECOMMENDATION===========
//===============Begin Declaration For QP_EXPECTED_EARNINGS================
typedef struct 
{
	short Q0EndDate;		//Quarter 0 end date
	double Q0ConEst;		//Quarter 0 Consensus Estimate
	long Q0ExpRptDt;		//Quarter 0 Expected Report Date
	short Flag;				//Earnings Flag

} *LPQP_EXPECTED_EARNINGS, QP_EXPECTED_EARNINGS;
//===============Begin Declaration For QP_EXPECTED_EARNINGS================
//===============Begin Declaration For QP_EARNINGS_ESTIMATES===============
typedef struct 
{
	long qr1EndDate;		//Quarter1 End Date (YYYYMM)
	double qr1CurrMean;		//Quarter 1 Current Mean
	long qr2EndDate;		//Quarter 2 End Date (YYYYMM)
	double qr2CurrMean;		//Quarter 2 Current Mean
	long fr1EndDate;		//Fiscal Year 1 End Date (YYYYMM)
	double fr1CurrMean;		//Fiscal Year 1 Current Mean
	long fr2EndDate;		//Fiscal Year 2 End Date (YYYYMM)
	double fr2CurrMean;		//Fiscal Year 2 Current Mean

	short qr1NumEst;		//Number of Estimates for Quarter 1
	short qr2NumEst;		//Number of Estimates for Quarter 2
	short fr1NumEst;		//Number of Estimates for Fiscal Year 1
	short fr2NumEst;		//Number of Estimates for Fiscal Year 2

	double qr1HighEst;		//High Estimate for Quarter 1
	double qr2HighEst;		//High Estimate for Quarter 2
	double fr1HighEst;		//High Estimate for Fiscal Year 1
	double fr2HighEst;		//High Estimate for Fiscal Year 2

	double qr1LowEst;		//Low Estimate for Quarter 1
	double qr2LowEst;		//Low Estimate for Quarter 2 
	double fr1LowEst;		//Low Estimate for Fiscal Year 1
	double fr2LowEst;		//Low Estimate for Fiscal Year 2
	
	double qr1YrPrActEPS;	//Quarter 1 Year Prior Actual EPS (Quarter 3)
	double qr2YrPrActEPS;	//Quarter 2 Year Prior Actual EPS (Quarter 2)
	double fr1YrPrActEPS;	/*Fiscal Year 1 Year Prior Actual 
									EPS (Fiscal Year 0)*/
		
	double qr1Growth;		/*Percent Growth, Quarter 1 Mean Over 
								Quarter 3 EPS*/
	double qr2Growth;		/*Percent Growth, Quarter 2 Mean Over 
								Quarter 2 EPS*/
	double fr1Growth;		/*Percent Growth, Fiscal Year 1 Mean Over 
								Fiscal Year 0 EPS*/
	double fr2Growth;		/*Percent Growth, Fiscal Year 2 Mean Over 
								Fiscal Year 1 Mean*/
	
	double LTGCurrMean;		//Long Term Growth Current Mean
	short NumLTGEst;		//Number of Long Term Growth Estimates
	double LTGHighEst;		//Long Term Growth High Estimate
	double LTGLowEst;		//Long Term Growth Low Estimate

} *LPQP_EARNINGS_ESTIMATES, QP_EARNINGS_ESTIMATES;
//===============End Declaration For QP_EARNINGS_ESTIMATES=================
//==============Begin Declaration For QP_ZACKS_DATA========================
typedef struct 
{
	char Briefing[1024];	//Briefing

	float Yr1EPSGrowth;		//1 Year EPS Growth
	float Yr5EPSGrowth;		//5 Year EPS Growth
	float Yr1ProjEPSGr;		//1 Year Projected EPS Growth
	float Yr2ProjEPSGr;		//2 Year Projected EPS Growth
	float Yr3to5ProjEPSGr;	//3 to 5 Year Projected EPS Growth
	float Yr1SalesGrowth;	//1 Year Sales Growth
	float Yr5SalesGrowth;	//5 Year Sales Growth
	float BookValuePerShare;//Book Value Per Share
	float TTMSales;			//Trailing Twelve Months Sales
	float Beta;				//Beta
	float TTMEPS;			//Trailing Twelve Months EPS
	float HiPERange;		//High PE Range
	float LoPERange;		//Low PE Range
	float PEG;				//Price/Earnings Growth
	float SharesOut;		//Shares Outstanding
	
	long  SharesShort;		//Shares Short
	
	float InstHolds;		//Institutional Holdings
	float LTDebtToEq;		//Long Term Debt To Equity
	float CashFlowPerShare;	//Cash Flow Per Share
	float ROE;				//Return On Equity
	float sharesFloat;	//The float for this issue   added 1/12/2000

	char  source;		//The source of the data - Z for Zacks or M for Market Guide  added 1/12/2000
	char  currency[18];  //The currency that the numbers are reported in

} *LPQP_ZACKS_DATA, QP_ZACKS_DATA;
//=============End Declaration For QP_ZACKS_DATA===========================
//======================Begin Declaration for QP_ZACKS_EPS=================
typedef struct 
{
	long FiscQuart;		//Date of the Fiscal Quarter
	
	float Sales;		//Sales of the Quarter
	float EPS;			//EPS of the Quarter

} *LPQP_ZACKS_EPS, QP_ZACKS_EPS;
//=========================End Declaration for QP_ZACKS_EPS================
//================Begin Declaration For EPSRANK============================
typedef struct
{
	long lDate;					//Rank Date

	unsigned char ucRank;		//EPS Ranking

} EPSRANK, *LPEPSRANK;
//==================End Declaration For EPSRANK============================
//=================Begin Declaration For QP_FUTURES_CONTRACT_REC===========
typedef struct 
{
	char LongSym[8];				//Long Symbol

	unsigned char ContractMonth;	//Contract Month
	
	short ContractYear;				//Contract Year

	long ExpireDate;				//Contract Expiration Date

	BOOL PContract;					//Is this a principle contract??

} *LPQP_FUTURES_CONTRACT_REC, QP_FUTURES_CONTRACT_REC;
//==================End Declaration For QP_FUTURES_CONTRACT_REC============
//==========Begin Declaration For QP_FUTURES_CASH_DESC_REC=================
typedef struct 
{
	char LongSym[8];		//Long Symbol
	char ShortSym[2];		//Short Symbol
	char Description[38];	//Cash Description

	long LastActiveDate;	//Last Active date for the commodity

} *LPQP_FUTURES_CASH_DESC_REC, QP_FUTURES_CASH_DESC_REC;
//======End Declaration For QP_FUTURES_CASH_DESC_REC=======================
//=======Begin Declaration For QP_FUTURES_COMMODITY_DESC_REC===============
typedef struct 
{
	char LongSym[8];		//Long Symbol
	char ShortSym[2];		//Short Symbol
	char Description[38];	//Commodity Description
	char Exchange[6];		//Exchange
	char ContractUnit[20];	//Contract Unit
	char ReportFreq[2];		//Report Frequency
	char TradeDays[7];		//Trad Days
	
	long Units;				/*Units - How the contract is 
									traded e.g. 100's, 128's*/
	long ValMult;			//??
	long LastMktVol;		//Last Market Value
	long LastMktOpenInt;	//Last Market Open Interest
	long LastMktVolDate;	//Last Market Volume Date
	long LastActiveDate;	//Last Active date for the commodity

	BOOL Grain;				//Traded in 100's of bushels or 1 bushel

} *LPQP_FUTURES_COMMODITY_DESC_REC, QP_FUTURES_COMMODITY_DESC_REC;
//========End Declaration For QP_FUTURES_COMMODITY_DESC_REC================
//=================Begin Declaration For QP_DATEREC_DATE===================
typedef struct 
{
	char month;		//Month
	char day;		//Day of month
	int  year;		//Year
	int  dayOfYear;	//Day of year
} *LPQPDATEREC_DATE, QP_DATEREC_DATE;
//================End Declaration For QP_DATEREC_DATE======================
//==============Begin Declaration For QP_DATEREC===========================
typedef struct  
{
	QP_DATEREC_DATE start_local;
	QP_DATEREC_DATE end_local;
	QP_DATEREC_DATE start_diskbuff;
	QP_DATEREC_DATE end_diskbuff;
	QP_DATEREC_DATE start_cd;
	QP_DATEREC_DATE end_cd;
} *LPQP_DATEREC, QP_DATEREC;
//==============Begin Declaration For QP_IDXCOMP_REC=======================
typedef struct 
{
	char Symbol[6];			//Symbol in Index
	char Index[6];			//Index

	float Weight;			//Weight of symbol in index

} *LPQP_IDXCOMP_REC, QP_IDXCOMP_REC;
//==============End Declaration For QP_IDXCOMP_REC=========================
//===============Begin Declaration For Expected Earnings====================
typedef struct 
{
	long High52Date;			//Date of the 52 week high value (MMDDYYYY)
	double High52;				//The 52 week high value
	long High5YrDate;			//Date of the 5 year high value (MMDDYYYY)
	double High5Yr;				//The 5 year high value

	long Low52Date;				//Date of the 52 week low value (MMDDYYYY)
	double Low52;				//The 52 week low value
	long Low5YrDate;			//Date of the 5 year low value (MMDDYYYY)
	double Low5Yr;				//The 5 year low value

} *LPQP_HIGH_LOW, QP_HIGH_LOW;
//===============End Declaration For Expected Earnings====================
//===============Begin Declaration For Name Changes ======================
typedef struct 
{
  short year;
  char  month;
  char  day;
  char  oldsym[6];
  char  oldname[30];
  char  newsym[6];
  char  newname[30];
} *LP_QP_NAME_CHANGES, QP_NAME_CHANGES;
//===============End Declaration For Name Changes ========================
//********************End Quotes Plus Data Records*************************
//================Begin Quotes Plus C-Interface Prototypes=================
#ifdef __cplusplus
extern "C" { 
#endif
//---------------Initialization and Termination----------------------------	
	//Initializes Quotes Plus Dll for current thread
	int __stdcall R2_InitReadStock( 
		const char *dir/*Quotes Plus Data Directory*/, 
		BOOL CheckForCD/*Use the Data CD*/,
		const char *CDBufferDir/*Not Used*/, 
		BOOL MinimizeOpens/*Not Used*/
		);
	
	//Terminates Quotes Plus Dll for current thread
	void __stdcall R2_Done();	
//-------------------Opening and Closing-----------------------------------
	int __stdcall R2_Open_Files( int iFileID/*File Identifier*/,
								 const char *dir/*Quotes Plus Data Directory*/ );
	
	int __stdcall R2_Close_Files( int iFileID/*File Identifier*/ );
//--------------------Read Price Data--------------------------------------
	int __stdcall R2_QP_LoadSymbol( 
		const char *sym/*Stock or Index symbol*/,
		LPSTOCK_RECORD lpStockRec/*Pointer to Price data record(s)*/,
		int max_rec/*Number of records/days to get(NOTE:lpStockRec must 
					 point to a valid memory block that can store 
					 max_rec * sizeof( Stock_Record) bytes of data*/,
		BOOL RAWDATA/*Load Unadjusted data*/, 
		BOOL IGNORE_HOLIDAYS/*Do not	return days that are holidays*/,
		LPMASTER_REC lpMasterRec/*Pointer to Master_Rec*/
		);
	
	int __stdcall R2_QP_Load_First_Symbol( 
		LPSTOCK_RECORD lpStockRec/*Pointer to Price data record(s)*/,
		int max_rec/*Number of records/days to get(NOTE:lpStockRec must 
					 point to a valid memory block that can store 
					 max_rec * sizeof( Stock_Record) bytes of data*/,
		BOOL IGNORE_HOLIDAYS/*Do not return days that are holidays*/,
		BOOL RAWDATA/*Load Unadjusted data*/, 
		LPMASTER_REC lpMasterRec/*Pointer to Master_Rec*/
		);
	
	int __stdcall R2_QP_Load_Next_Symbol( 
		LPSTOCK_RECORD lpStockRec/*Pointer to Price data record(s)*/,
		int max_rec/*Number of records/days to get(NOTE:lpStockRec must 
					 point to a valid memory block that can store 
					 max_rec * sizeof( Stock_Record) bytes of data*/,
		BOOL IGNORE_HOLIDAYS/*Do not return days that are holidays*/,
		BOOL RAWDATA/*Load Unadjusted data*/, 
		LPMASTER_REC lpMasterRec/*Pointer to Master_Rec*/
		);
	
	int __stdcall R2_QP_LoadWkSymbol( 
		const char *sym/*Weekly symbol*/,
		LPSTOCK_RECORD lpStockRec/*Pointer to Price data record(s)*/,
		int max_rec/*Number of records/days to get(NOTE:lpStockRec must 
					 point to a valid memory block that can store 
					 max_rec * sizeof( Stock_Record) bytes of data*/,
		BOOL RAWDATA/*Load Unadjusted data*/
		);

	int __stdcall R2_QP_Load_First_WkSymbol( 
		LPSTOCK_RECORD lpStockRec/*Pointer to Price data record(s)*/,
		int max_rec/*Number of records/days to get(NOTE:lpStockRec must 
					 point to a valid memory block that can store 
					 max_rec * sizeof( Stock_Record) bytes of data*/
		);
	
	int __stdcall R2_QP_Load_Next_WkSymbol( 
		LPSTOCK_RECORD lpStockRec/*Pointer to Price data record(s)*/,
		int max_rec/*Number of records/days to get(NOTE:lpStockRec must 
					 point to a valid memory block that can store 
					 max_rec * sizeof( Stock_Record) bytes of data*/
		);
	
	int __stdcall R2_QP_LoadFundSymbol( 
		const char *sym/*Mutual Fund Symbol*/,
		LPSTOCK_RECORD lpStockRec/*Pointer to Price data record(s)*/,
		int max_rec/*Number of records/days to get(NOTE:lpStockRec must 
					 point to a valid memory block that can store 
					 max_rec * sizeof( Stock_Record) bytes of data*/,
		BOOL RAWDATA/*Load Unadjusted data*/,
		BOOL IGNORE_HOLIDAYS/*Do not return days that are holidays*/
		);
	
	int __stdcall R2_QP_Load_First_Fund_Symbol( 
		LPSTOCK_RECORD lpStockRec/*Pointer to Price data record(s)*/,
		int max_rec/*Number of records/days to get(NOTE:lpStockRec must 
					 point to a valid memory block that can store 
					 max_rec * sizeof( Stock_Record) bytes of data*/,
		BOOL IGNORE_HOLIDAYS/*Do not return days that are holidays*/,
		BOOL RAWDATA/*Load Unadjusted data*/
		);
	
	int __stdcall R2_QP_Load_Next_Fund_Symbol( 
		LPSTOCK_RECORD lpStockRec/*Pointer to Price data record(s)*/,
		int max_rec/*Number of records/days to get(NOTE:lpStockRec must 
					 point to a valid memory block that can store 
					 max_rec * sizeof( Stock_Record) bytes of data*/,
		BOOL IGNORE_HOLIDAYS/*Do not return days that are holidays*/,
		BOOL RAWDATA/*Load Unadjusted data*/
		);
//-------------------Read Fundamental Data---------------------------------
	int __stdcall R2_QP_ReadMaster( 
		int NavigateID/*Navigation Identifier*/,
		int IndexID/*Index Identifier*/,
		const char *cpKey,/*Search Key*/
		LPMASTER_REC lpMasterRec/*Pointer to Master_Rec*/
		);
	
	int __stdcall R2_QP_ReadFundMaster( 
		int NavigateID/*Navigation Identifier*/,
		int IndexID/*Index Identifier*/,
		const char *cpKey/*Search Key*/,
		LPMASTER_REC lpMasterRec/*Pointer to Master_Rec*/
		);
		
	int __stdcall R2_QP_ReadSplit( 
		int NavigateID/*Navigation Identifier*/,
		const char *cpKey/*Search Key*/,
		LPSPLIT_REC lpSplitRec/*Pointer to a SplitRec*/
		);
	
	int __stdcall R2_QP_ReadDistributions( 
		int NavigateID/*Navigation Identifier*/,
		int IndexID/*Index Identifier*/,
		const char *cpKey/*Search Key*/,
		long lExDate/*Expiration Date*/,
		LPQP_DISTRIB_REC lpqpDistribRec/*Pointer to a QP_DISTRIB_REC*/
		);
	
	int __stdcall R2_QP_ReadIrlFile( 
		int NavigateID/*Navigation Identifier*/,
		int IndexID/*Index Identifier*/,
		const char *cpKey/*Search Key*/,
		LPQP_IRL_REC lpqpIrlRec/*Pointer to a QP_IRL_REC*/
		);

	int __stdcall R2_QP_ReadSICFile( 
		int NavigateID/*Navigation Identifier*/,
		int IndexID/*Index Identifier*/,
		const char *cpKey/*Search Key*/,
		LPSIC_REC lpSicRec/*Pointer to a Sic_Rec*/
		);
	
	int __stdcall R2_QP_ReadDividends( 
		const char* cpSymbol/*Ticker Symbol*/,
		Dividend_Rec Divs[4]/*Dividends Array*/
		);

	int __stdcall R2_QP_ReadEEC( 
		const char* cpSymbol/*Ticker Symbol*/,
		LPQP_EARNINGS_EST_CON lpqpEecRec/*Pointer to 
										   	a QP_EARNINGS_EST_CON*/
		);  
	
	int __stdcall R2_QP_ReadAnaRec( 
		const char* cpSymbol/*Ticker Symbol*/,
		LPQP_ANALYST_RECOMMENDATION lpqpAnRec/*Pointer to a 
											 QP_ANALYST_RECOMMENDATION*/
		);

	int __stdcall R2_QP_ReadExpEarn( 
		const char* cpSymbol/*Ticker Symbol*/,
		LPQP_EXPECTED_EARNINGS lpqpExpEarnRec/*Pointer to a 
											 QP_EXPECTED_EARNINGS*/
		);

	int __stdcall R2_QP_ReadEarnEst( 
		const char* cpSymbol/*Ticker Symbol*/,
		LPQP_EARNINGS_ESTIMATES lpqpEarnEst/*Pointer to a 
											 QP_EARNINGS_ESTIMATES*/
		);

	int __stdcall R2_QP_ReadZacksData( 
		const char* cpSymbol/*Ticker Symbol*/,
		LPQP_ZACKS_DATA lpqpZacksData/*Pointer to a QP_ZACKS_DATA*/
		);  

	int __stdcall R2_QP_ReadZacksEPS( 
		const char* cpSymbol/*Ticker Symbol*/,
		QP_ZACKS_EPS zacks_EPS[20]/*Zack Eps record array*/
		); 

	int __stdcall R2_QP_ReadEpsRank( const char* cpSymbol, /*The symbol that youre looking for*/
		LPEPSRANK lpEpsRank, /*Pointer to an eps rank array */
		int iRecords /* Number of records to read*/);

	int __stdcall R2_QP_ReadHighLow( 
		const char* pSymbol/*Ticker Symbol*/,
		LPQP_HIGH_LOW lpHighLowRec/*Pointer to a QP_HIGH_LOW record*/ );
//------------------------Commodities--------------------------------------
	BOOL __stdcall GetFutureContractRecord( 
		int NavigateID/*Navigation Identifier*/,
		const char* cpLongSym/*Commodity Long Symbol*/,
		int iContractMonth/*Contract Month*/,
		int iContractYear/*Contract Year*/,
		LPQP_FUTURES_CONTRACT_REC lpqpContract/*Pointer to a 
												 QP_FUTURES_CONTRACT_REC*/
		);

	BOOL __stdcall GetFutureCashRecord( 
		int NavigateID/*Navigation Identifier*/,
		int IndexID/*Index Identifier*/,
		const char *cpKey/*Search Key*/,
		LPQP_FUTURES_CASH_DESC_REC lpqpCash/*Pointer to a 
												QP_FUTURES_CASH_DESC_REC*/
		);	
	
	BOOL __stdcall GetFutureCommodityRecord( 
		int NavigateID/*Navigation Identifier*/,
		int IndexID/*Index Identifier*/,
		const char *cpKey/*Search Key*/,
		LPQP_FUTURES_COMMODITY_DESC_REC lpqpDesc/*Pointer to a
											QP_FUTURES_COMMODITY_DESC_REC*/
		);
//-----------------------------Other---------------------------------------
	void __stdcall R2_QP_Get_Cur_Symbol( 
		char *pSymbol/*Pointer to receive current symbol - must be at
						least 7 bytes long*/
		);
	void __stdcall R2_GetDates( 
		const char *cpSymbol/*Ticker Symbol*/, 
		LPQP_DATEREC dates/*Pointer to a dateRec*/,
		int iDateID/*Date Identifier*/, 
		int iFileID/*File Identifier*/
		);
	
	int __stdcall R2_QP_ReadIdxComponent( 
		int NavigateID/*Navigation Identifier*/,
		int IndexID/*Index Identifier*/,
		const char* cpSym/*Ticker Symbol*/, 
		const char* cpIndex/*Index Symbol*/,
		LPQP_IDXCOMP_REC lpqpIdxComp/*Pointer to a QP_IDXCOMP_REC*/
		);

	int __stdcall R2_QP_ReadNameChg( int FirstNext, /*Navigation Identifier*/
									 int index, /*Index Identifier - QP_OLDSYMBOL or QP_NEWSYMBOL */
									 const char *pText, /*Ticker Symbol*/
									 void *pUsrNameChg /*Pointer to a QP_NAME_CHANGES*/ );
#ifdef __cplusplus
}
#endif
//=================End Quotes Plus C-Interface Prototypes================== 
#pragma pack( pop, _USRDLL11_H_, 1 )
#endif
