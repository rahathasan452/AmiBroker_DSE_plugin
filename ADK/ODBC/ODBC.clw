; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
ClassCount=3
Class1=CODBCApp
LastClass=CConfigDlg
NewFileInclude2=#include "ODBC.h"
ResourceCount=1
NewFileInclude1=#include "stdafx.h"
Class2=CConfigDlg
LastTemplate=CRecordset
Class3=CSRecordset
Resource1=IDD_CONFIG_DIALOG

[CLS:CODBCApp]
Type=0
HeaderFile=ODBC.h
ImplementationFile=ODBC.cpp
Filter=N
LastObject=CODBCApp
BaseClass=CWinApp
VirtualFilter=AC

[DLG:IDD_CONFIG_DIALOG]
Type=1
Class=CConfigDlg
ControlCount=35
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342308352
Control5=IDC_TABLE_NAME_COMBO,combobox,1344340226
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,button,1342177287
Control8=IDC_STATIC,static,1342308352
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352
Control12=IDC_STATIC,static,1342308352
Control13=IDC_STATIC,static,1342308352
Control14=IDC_STATIC,static,1342308352
Control15=IDC_STATIC,static,1342308352
Control16=IDC_SYMBOL_COMBO,combobox,1344340226
Control17=IDC_STATIC,button,1342177287
Control18=IDC_STATIC,static,1342308352
Control19=IDC_STATIC,static,1342308352
Control20=IDC_SQL_QUOTATIONS_EDIT,edit,1484849280
Control21=IDC_SQL_SYMBOL_LIST_EDIT,edit,1484849280
Control22=IDC_DATE_COMBO,combobox,1344340226
Control23=IDC_OPEN_COMBO,combobox,1344340226
Control24=IDC_HIGH_COMBO,combobox,1344340226
Control25=IDC_LOW_COMBO,combobox,1344340226
Control26=IDC_CLOSE_COMBO,combobox,1344340226
Control27=IDC_VOLUME_COMBO,combobox,1344340226
Control28=IDC_OPEN_INT_COMBO,combobox,1344340226
Control29=IDC_SQL_CUSTOM_QUERIES_CHECK,button,1342242819
Control30=IDC_CONNECT_BUTTON,button,1342242816
Control31=IDC_DSN_EDIT,edit,1352728644
Control32=IDC_RETRIEVE_BUTTON,button,1342242816
Control33=IDC_AUTO_REFRESH_CHECK,button,1342242819
Control34=IDC_STATIC,static,1342308352
Control35=IDC_SERVER_TYPE_COMBO,combobox,1344339971

[CLS:CConfigDlg]
Type=0
HeaderFile=ConfigDlg.h
ImplementationFile=ConfigDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_SQL_CUSTOM_QUERIES_CHECK
VirtualFilter=dWC

[CLS:CSRecordset]
Type=0
HeaderFile=SRecordset.h
ImplementationFile=SRecordset.cpp
BaseClass=CRecordset
Filter=N
VirtualFilter=r
LastObject=CSRecordset

[DB:CSRecordset]
DB=1
DBType=ODBC
ColumnCount=0

