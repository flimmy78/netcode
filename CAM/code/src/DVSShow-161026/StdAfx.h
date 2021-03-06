// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__62517DA5_C9E0_417B_BE47_A293647365E5__INCLUDED_)
#define AFX_STDAFX_H__62517DA5_C9E0_417B_BE47_A293647365E5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxtempl.h>
#include <afxsock.h>

#include <map>
using namespace std;

#include ".\LeftCoolBar\sizecbar.h"
#include ".\LeftCoolBar\scbarg.h"

#define _BCGCBPRO_STATIC_
#include <BCGCBProInc.h>			// BCGControlBar Pro
#define CDialog CBCGPDialog
#include "VideoComDef.h"
#include "VideoCommonDLL.h"
#include "PublicStruct.h"
#include "netsdk.h"
#include "H264Play.h"

#include <iostream>  
#include <sstream> 
#include <vector>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__62517DA5_C9E0_417B_BE47_A293647365E5__INCLUDED_)
