/*	$NetBSD: StdAfx.h,v 1.6 2022/09/23 12:15:26 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/* stdafx.h : include file for standard system include files, */
/*  or project specific include files that are used frequently, but */
/*      are changed infrequently */
/* */

/*
 * Minimum version is Windows 8 and Windows Server 2012
 */
#define _WIN32_WINNT  0x0602
#define NTDDI_VERSION 0x06020000

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif /* ifndef _CRT_SECURE_NO_DEPRECATE */

#if !defined(AFX_STDAFX_H__61537819_39FC_11D3_A97A_00105A12BD65__INCLUDED_)
#define AFX_STDAFX_H__61537819_39FC_11D3_A97A_00105A12BD65__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

#define VC_EXTRALEAN /* Exclude rarely-used stuff from Windows headers */

#include <afxdtctl.h> /* MFC support for Internet Explorer 4 Common Controls */
#include <afxext.h>   /* MFC extensions */
#include <afxwin.h>   /* MFC core and standard components */
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h> /* MFC support for Windows Common Controls */
#endif		    /* _AFX_NO_AFXCMN_SUPPORT */

/*{{AFX_INSERT_LOCATION}} */
/* Microsoft Visual C++ will insert additional declarations immediately before
 */
/* the previous line. */

#endif /* !defined(AFX_STDAFX_H__61537819_39FC_11D3_A97A_00105A12BD65__INCLUDED_) \
	*/
