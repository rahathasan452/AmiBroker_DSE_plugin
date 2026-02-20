// ConfigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "QP2.h"
#include "ConfigDlg.h"
#include "usrdll22.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog


CConfigDlg::CConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfigDlg)
	m_bCash = FALSE;
	m_bCommodity = FALSE;
	m_bFund = FALSE;
	m_bIndex = TRUE;
	m_bSector = TRUE;
	m_bStock = TRUE;
	m_iContType = 0;
	m_bRawData = FALSE;
	m_bOnlyActivelyTraded = FALSE;
	m_bExcludeNoQuotes = TRUE;
	//}}AFX_DATA_INIT
	m_bRawData = AfxGetApp()->GetProfileInt( "QP2", "RawData", FALSE );
}


void CConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigDlg)
	DDX_Control(pDX, IDC_STATUS_STATIC, m_oStatusStatic);
	DDX_Check(pDX, IDC_CASH_CHECK, m_bCash);
	DDX_Check(pDX, IDC_COMMODITY_CHECK, m_bCommodity);
	DDX_Check(pDX, IDC_FUND_CHECK, m_bFund);
	DDX_Check(pDX, IDC_INDEX_CHECK, m_bIndex);
	DDX_Check(pDX, IDC_SECTOR_CHECK, m_bSector);
	DDX_Check(pDX, IDC_STOCK_CHECK, m_bStock);
	DDX_CBIndex(pDX, IDC_CONT_TYPE_COMBO, m_iContType);
	DDX_Check(pDX, IDC_UNADJUSTED_CHECK, m_bRawData);
	DDX_Check(pDX, IDC_ONLYACTIVELYTRADED_CHECK, m_bOnlyActivelyTraded);
	DDX_Check(pDX, IDC_EXCLUDE_NO_QUOTES_CHECK, m_bExcludeNoQuotes);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigDlg, CDialog)
	//{{AFX_MSG_MAP(CConfigDlg)
	ON_BN_CLICKED(IDC_RETRIEVE_BUTTON, OnRetrieveButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg message handlers

void RemTrailSpace( char *string, int len )
{
	char *c = string + len;

	*c-- = '\0';

	while( *c == ' ' ) *c-- = '\0';

	return;
}

void CConfigDlg::OnRetrieveButton() 
{
	UpdateData( TRUE );

	SetupGroups();

	if( m_bStock || m_bIndex ) RetrieveStocks();

	if( m_bFund ) RetrieveFunds();

	if( m_bCash ) RetrieveCashFutures();

	if( m_bCommodity ) RetrieveCommodityFutures();

	if( m_bSector ) RetrieveSectors();


	m_oStatusStatic.SetWindowText( "Done" );

}

void CConfigDlg::AddComponents(QP_IRL_REC *pIrlRec, InfoSite *pSite)
{
	QP_IDXCOMP_REC	idxCompRec;				// Index Component Record
	Master_Rec		Master;					// Master record
	int	erc;

	char cpIrlIdx[1024] , tmptxt[1024];
	
	//Create IRL Index
	sprintf( cpIrlIdx, "!ID%03d", pIrlRec->GroupID );
	
	erc = R2_QP_ReadIdxComponent( QP_EQUAL, QP_INDEX, "", cpIrlIdx,	&idxCompRec );	//Reads From The Index Components Table
	
	while( (erc == 0) && (strnicmp( idxCompRec.Index, cpIrlIdx, 6 ) == 0) )
	{
		// Add the symbol to the issue box.
		memcpy( tmptxt, idxCompRec.Symbol, sizeof( idxCompRec.Symbol ) );
		tmptxt[sizeof(idxCompRec.Symbol)] = 0;
		
		// Add the name to the issue box
		if( R2_QP_ReadMaster( QP_EQUAL, QP_SYMBOL, tmptxt, &Master ) == 0 && 
			Master.TickerType != qp_ticker_invalid &&
			( ! m_bOnlyActivelyTraded || Master.issue_status == '0' ) &&
			( ! m_bExcludeNoQuotes || HasQuotes( &Master ) ) )
		{
			struct StockInfoFormat4 *si = AddStock( &Master );

			if( si )
			{
				si->IndustryID = pIrlRec->GroupID;
			}
		}
		erc = R2_QP_ReadIdxComponent( QP_NEXT, QP_INDEX, "", cpIrlIdx,
			&idxCompRec );	//Reads From The Index Components Table
	}
}

struct StockInfoFormat4 * CConfigDlg::AddStock(Master_Rec *pRec)
{
	char symbol[ 16 ];

	memcpy( symbol, pRec->symbol, sizeof(pRec->symbol) );
	RemTrailSpace( symbol, sizeof(pRec->symbol)  );

	struct StockInfoFormat4 *si = m_pSite->AddStock( symbol );

	int iMarketID = 0;
	if ( strncmp( pRec->exchange , "01" , 2 ) == 0 ) iMarketID = 1;
	if ( strncmp( pRec->exchange , "02" , 2 ) == 0 ) iMarketID = 2;
	if ( strncmp( pRec->exchange , "60" , 2 ) == 0 ) iMarketID = 3;
	if ( strncmp( pRec->exchange , "65" , 2 ) == 0 ) iMarketID = 4;
	if ( strncmp( pRec->exchange , "40" , 2 ) == 0 ) iMarketID = 5;

    si->Flags &= 0xFFFF0002;
    si->Flags |= (( iMarketID << 1 ) & 0x0FFFC ) + ( iMarketID & 1L );

	if( si )
	{
		RemTrailSpace( pRec->issuer_desc, sizeof( pRec->issuer_desc ) - 1 );
		strcpy( si->FullName, pRec->issuer_desc );
		RemTrailSpace( pRec->issue_desc, sizeof( pRec->issue_desc ) - 1 );
		if( strlen( pRec->issue_desc ) > 0 )
		{
			strcat( si->FullName, "-" );
			strcat( si->FullName, pRec->issue_desc );
		}
	}

	return si;
}

BOOL CConfigDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigDlg::RetrieveStocks()
{
	int count;

	Master_Rec Rec;
    int rc ;

/*
From QuotesPlus:
issue_status

0 = actively trading
 
1, P = trading on a when issued basis
 
5, 6, 7, A, B, C, D, E, M = not trading
 
4, N = new symbol
 
G, K, X, R, Z = changes to symbol, cusip, name, etc.
 
Usually anything with a 0 is good.  
Sometimes we get master records for symbols that we do not get data for.  
I try to get rid of these about once a week.  
Let me know if you need more information.  

  
issue_type:
A index 
0 common 
1 preferred 
F ADR 
2 warrant or right 
7 mutual or investment trust fund 
G convertible preferred 
% ETF 
E Unit - (UIT) 
8 certificate 
4 Unit 

*/

	count = 0;

	CString oString;
	
	rc = R2_QP_ReadMaster( QP_TOP , QP_SYMBOL , "" , &Rec ); // Start at the first stock

	while ( ( rc == 0 ) )
	{
		if( Rec.TickerType != qp_ticker_invalid &&
			( ! m_bOnlyActivelyTraded || Rec.issue_status == '0' ) &&
			( ! m_bExcludeNoQuotes || HasQuotes( &Rec ) ) )  // invalid symbol ?
		{
			if( ( m_bIndex && Rec.symbol[ 0 ] == '!' ) ||
				( m_bStock && Rec.symbol[ 0 ] != '!' ) )
			{
				struct StockInfoFormat4 *si = AddStock( &Rec );

				if( Rec.symbol[ 0 ] == '!' )
				{
					si->MoreFlags |= 1; // index flag
				}
				
				int group = 0;  // undefined

				switch( Rec.issue_type )
				{
				case 'A':
					group = 1;
					break;
				case '0':
					group = 2;
					break;
				case '1':
					group = 3;
					break;
				case 'F':
					group = 4;
					break;
				case '2':
					group = 5;
					break;
				case '7':
					group = 6;
					break;
				case 'G':
					group = 7;
					break;
				case '%':
					group = 8;
					break;
				case 'E':
					group = 9;
					break;
				case '8':
					group = 10;
					break;
				case '4':
					group = 11;
					break;
				default:
					break;
				}

				si->Flags &= 0x0000FFFF;
				si->Flags |= group<<16;	// 
			
				oString.Format("Retrieving symbol %d. - %s", count, si->ShortName );

				m_oStatusStatic.SetWindowText( oString );
				m_oStatusStatic.UpdateWindow();
			}
		}

		rc = R2_QP_ReadMaster( QP_NEXT , QP_SYMBOL , Rec.symbol , &Rec ); // Get the next record in symbol order

		count++;
	}

}

void CConfigDlg::RetrieveFunds()
{
	int count;

	Master_Rec Rec;
    int rc ;

	count = 0;

	CString oString;
	
	rc = R2_QP_ReadFundMaster( QP_TOP , QP_SYMBOL , "" , &Rec ); // Start at the first stock

	while ( ( rc == 0 ) )
	{
		if( ! m_bExcludeNoQuotes || HasQuotes( &Rec, TRUE ) )
		{
			struct StockInfoFormat4 *si = AddStock( &Rec );

			si->Flags &= ~2; // remove continuous flag

			si->Flags &= 0x0000FFFF;

			int group = 0;  // undefined

			switch( Rec.issue_type )
			{
			case 'A':
				group = 1;
				break;
			case '0':
				group = 2;
				break;
			case '1':
				group = 3;
				break;
			case 'F':
				group = 4;
				break;
			case '2':
				group = 5;
				break;
			case '7':
				group = 6;
				break;
			case 'G':
				group = 7;
				break;
			case '%':
				group = 8;
				break;
			case 'E':
				group = 9;
				break;
			case '8':
				group = 10;
				break;
			case '4':
				group = 11;
				break;
			default:
				break;
			}

			si->Flags |= group<<16;	// 

			oString.Format("Retrieving fund symbol %d. - %s", count, si->ShortName );

			m_oStatusStatic.SetWindowText( oString );
			m_oStatusStatic.UpdateWindow();
		}

		rc = R2_QP_ReadFundMaster( QP_NEXT , QP_SYMBOL , Rec.symbol , &Rec ); // Get the next record in symbol order

		count++;
	}

}

void CConfigDlg::RetrieveSectors()
{
	char LastSector[128] , ThisSector[128] , idx[128] , tmptxt[1024];
	QP_IRL_REC	irlRec;				// IRL data record

	CString oString;

	int rc;

	int iSector = 1;

    LastSector[0] = 0;
    ThisSector[0] = 0;
    idx[0] = 0;

	m_pSite->SetCategoryName( CATEGORY_SECTOR, 0, "Undefined" );
	m_pSite->SetCategoryName( CATEGORY_INDUSTRY, 0, "Undefined" );
	m_pSite->SetIndustrySector( 0, 0 );




	m_pSite->SetCategoryName( CATEGORY_MARKET, 0, "Undefined" );
	m_pSite->SetCategoryName( CATEGORY_MARKET, 1, "NYSE" );
	m_pSite->SetCategoryName( CATEGORY_MARKET, 2, "Amex" );
	m_pSite->SetCategoryName( CATEGORY_MARKET, 3, "Nasdaq" );
	m_pSite->SetCategoryName( CATEGORY_MARKET, 4, "OTC" );
	m_pSite->SetCategoryName( CATEGORY_MARKET, 5, "Mutual funds" );

	rc = R2_QP_ReadIrlFile( QP_TOP, QP_SECTOR, "" , &irlRec );
    while( rc == 0 )
    {
		memcpy( ThisSector , irlRec.Sector , sizeof(irlRec.Sector) );
		
		memcpy( tmptxt , irlRec.Industry , sizeof(irlRec.Industry) );
					
		RemTrailSpace( ThisSector, sizeof(irlRec.Sector) );
		RemTrailSpace( tmptxt, sizeof(irlRec.Industry) );
		
		if ( strcmp( ThisSector , LastSector ) != 0 ) // This is a new sector
		{
			iSector++;
			m_pSite->SetCategoryName( CATEGORY_SECTOR, iSector, ThisSector );

			oString.Format("Retrieving sector: %s", ThisSector );

			m_oStatusStatic.SetWindowText( oString );
			m_oStatusStatic.UpdateWindow();
		}

		strcpy( LastSector , ThisSector );
		
		m_pSite->SetCategoryName( CATEGORY_INDUSTRY, irlRec.GroupID, tmptxt );

		m_pSite->SetIndustrySector( irlRec.GroupID, iSector );
		
		AddComponents( &irlRec, m_pSite);

		rc = R2_QP_ReadIrlFile( QP_NEXT , QP_SECTOR , "", &irlRec );

    }


}

void CConfigDlg::RetrieveCashFutures()
{
	int count;
	char symbol[ 16 ];

	QP_FUTURES_CASH_DESC_REC Rec;
    int rc ;

	count = 0;

	CString oString;
	
	rc = GetFutureCashRecord( QP_TOP , QP_LONGSYM , "" , &Rec ); // Start at the first stock

	while ( ( rc == 1 ) )
	{
		sprintf( symbol, "%c%c1599", Rec.ShortSym[ 0 ], Rec.ShortSym[ 1 ] );

		if( 1 /* ( ! m_bExcludeNoQuotes || HasQuotes( symbol ) ) */ ) // this caused problems hence commented out
		{
			struct StockInfoFormat4 *si = m_pSite->AddStock( symbol );
			
			strncpy( si->FullName, Rec.Description, sizeof( Rec.Description ) );
			RemTrailSpace( si->FullName, sizeof( Rec.Description ) );

			si->Flags |= ( 12 << 16 ); // 12th group

			oString.Format("Retrieving futures cash symbol %d. - %s", count, si->ShortName );

			m_oStatusStatic.SetWindowText( oString );
			m_oStatusStatic.UpdateWindow();
		}

		rc = GetFutureCashRecord( QP_NEXT , QP_NAME, "" , &Rec ); // Get the next record in symbol order

		count++;
	}

}

void CConfigDlg::RetrieveCommodityFutures()
{
	int count;

	QP_FUTURES_COMMODITY_DESC_REC Rec;
    int rc ;

	count = 0;

	char symbol[16];

	CString oString;
	
//#define QP_SYMEXDATE	19		//Symbol + Expiration Date
//#define QP_LONGSYM		20		//Long Symbol - Used For Futures

	rc = GetFutureCommodityRecord( QP_TOP , QP_LONGSYM , "" , &Rec ); // Start at the first stock

	while ( ( rc == 1 ) )
	{
		sprintf( symbol, "%c%c16%02d", Rec.ShortSym[ 0 ], Rec.ShortSym[ 1 ], m_iContType );

		if( 1 /*! m_bExcludeNoQuotes || HasQuotes( symbol ) */ ) // this caused problems hence commented out !
		{

			struct StockInfoFormat4 *si = m_pSite->AddStock( symbol );
			
			strncpy( si->FullName, Rec.Description, sizeof( Rec.Description ) );
			RemTrailSpace( si->FullName, sizeof( Rec.Description ) );

			si->Flags |= ( 13 << 16 ); // 13th group

			oString.Format("Retrieving futures comodity symbol %d. - %s", count, si->ShortName );

			m_oStatusStatic.SetWindowText( oString );
			m_oStatusStatic.UpdateWindow();

		}

		rc = GetFutureCommodityRecord( QP_NEXT , QP_LONGSYM, Rec.LongSym , &Rec ); // Get the next record in symbol order

		count++;
	}


}

/*
void CConfigDlg::RetrieveContractFutures()
{
	int count;

	QP_FUTURES_CONTRACT_REC Rec;
    int rc ;

	count = 0;

	CString oString;
	
//#define QP_SYMEXDATE	19		//Symbol + Expiration Date
//#define QP_LONGSYM		20		//Long Symbol - Used For Futures

	rc = GetFutureContractRecord( QP_TOP, "" , 1, 1, &Rec ); // Start at the first stock

	while ( ( rc == 1 ) )
	{
		if( 1 )
		{
			char symbol[ 16 ];

			sprintf( symbol, "%2s1599", Rec.ShortSym, Rec. );

			struct StockInfo *si = m_pSite->AddStock( symbol );
			
			strncpy( si->FullName, Rec.Description, sizeof( Rec.Description ) );
			RemTrailSpace( si->FullName, sizeof( Rec.Description ) );

			si->Flags |= ( 3 << 16 ); // 3rd group

			oString.Format("Retrieving futures comodity symbol %d. - %s", count, si->ShortName );

			m_oStatusStatic.SetWindowText( oString );
			m_oStatusStatic.UpdateWindow();

		}

		rc = GetFutureCommodityRecord( QP_NEXT, symbol, &Rec ); // Get the next record in symbol order

		count++;
	}
}
*/

void CConfigDlg::OnOK() 
{
	CDialog::OnOK();
	AfxGetApp()->WriteProfileInt( "QP2", "RawData", m_bRawData );
}

void CConfigDlg::SetupGroups()
{
	m_pSite->SetCategoryName( CATEGORY_GROUP, 0, "Undefined" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 1, "Index" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 2, "Common Stocks" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 3, "Preferred stocks" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 4, "ADR" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 5, "Warrant or right" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 6, "Mutual or inv. trust fund" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 7, "Convertible preferred" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 8, "ETF" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 9, "Unit (UIT)" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 10, "Certificate" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 11, "Unit" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 12, "Futures (cash)" );
	m_pSite->SetCategoryName( CATEGORY_GROUP, 13, "Futures (commodity)" );
}

BOOL CConfigDlg::HasQuotes(Master_Rec *pRec, BOOL bFund)
{
	char symbol[ 16 ];

	memcpy( symbol, pRec->symbol, sizeof(pRec->symbol) );
	RemTrailSpace( symbol, sizeof(pRec->symbol)  );

	return HasQuotes( symbol, bFund );
}

BOOL CConfigDlg::HasQuotes(LPCTSTR pszSymbol, BOOL bFund)
{
	Stock_Record aRec;
   	Master_Rec MasterRec;				//Quotes Plus Master Record

	int nQty = 0;
	
	if( bFund )
	{
		nQty = R2_QP_LoadFundSymbol( 
					   pszSymbol,
					   &aRec/*Pointer to Price data record(s)*/,
					   1 /*Number of records/days to get(NOTE:lpStockRec must 
				 point to a valid memory block that can store 
				 max_rec * sizeof( Stock_Record) bytes of data*/,
					FALSE/*Load Unadjusted data*/, 
					TRUE/*Do not	return days that are holidays*/ );
	}
	else
	{
		nQty = R2_QP_LoadSymbol( 
						   pszSymbol,
						   &aRec/*Pointer to Price data record(s)*/,
						   1 /*Number of records/days to get(NOTE:lpStockRec must 
					 point to a valid memory block that can store 
					 max_rec * sizeof( Stock_Record) bytes of data*/,
						FALSE/*Load Unadjusted data*/, 
						TRUE/*Do not	return days that are holidays*/,
						&MasterRec/*Pointer to Master_Rec*/
		);
	}

	return nQty > 0;
}
