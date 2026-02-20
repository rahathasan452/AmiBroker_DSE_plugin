; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CConfigDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "qt.h"
LastPage=0

ClassCount=2
Class1=CConfigDlg
Class2=CQTApp

ResourceCount=2
Resource1=IDD_CONFIG_DIALOG (English (U.S.))
Resource2=IDR_STATUS_MENU (English (U.S.))

[CLS:CConfigDlg]
Type=0
BaseClass=CDialog
HeaderFile=ConfigDlg.h
ImplementationFile=ConfigDlg.cpp
Filter=D
VirtualFilter=dWC
LastObject=CConfigDlg

[CLS:CQTApp]
Type=0
BaseClass=CWinApp
HeaderFile=QT.h
ImplementationFile=QT.cpp
Filter=N
VirtualFilter=AC
LastObject=CQTApp

[DLG:IDD_CONFIG_DIALOG]
Type=1
Class=CConfigDlg

[DLG:IDD_CONFIG_DIALOG (English (U.S.))]
Type=1
Class=CConfigDlg
ControlCount=17
Control1=IDC_SERVER_EDIT,edit,1350631552
Control2=IDC_PORT_EDIT,edit,1350639744
Control3=IDC_INTERVAL_EDIT,edit,1350639744
Control4=IDC_AUTOSYMBOLS_CHECK,button,1342242851
Control5=IDC_MAXSYMBOL_EDIT,edit,1350639744
Control6=IDC_OPTIMIZED_INTRADAY_CHECK,button,1342242851
Control7=IDC_TIMESHIFT_EDIT,edit,1350631552
Control8=IDC_RETRIEVE_BUTTON,button,1342242816
Control9=IDOK,button,1342242817
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352
Control12=IDCANCEL,button,1342242816
Control13=IDC_STATIC,static,1342308352
Control14=IDC_STATUS_STATIC,static,1342312448
Control15=IDC_STATIC,button,1342177287
Control16=IDC_STATIC,static,1342308352
Control17=IDC_STATIC,static,1342308352

[MNU:IDR_STATUS_MENU (English (U.S.))]
Type=1
Class=?
Command1=ID_STATUS_RECONNECT
Command2=ID_STATUS_SHUTDOWN
Command3=ID_STATUS_RECONNECT
Command4=ID_STATUS_SHUTDOWN
Command5=ID_STATUS_REFILL
Command6=ID_STATUS_ADDSYMBOL
CommandCount=6

