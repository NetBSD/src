/*	$NetBSD: VersionInfo.h,v 1.4 2022/09/23 12:15:26 christos Exp $	*/

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

/* VersionInfo.h: interface for the CVersionInfo class. */
/* */
/*//////////////////////////////////////////////////////////////////// */

#if !defined(AFX_VERSIONINFO_H__F82E9FF3_5298_11D4_AB87_00C04F789BA0__INCLUDED_)
#define AFX_VERSIONINFO_H__F82E9FF3_5298_11D4_AB87_00C04F789BA0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

class CVersionInfo {
      public:
	CVersionInfo(CString filename);
	virtual ~CVersionInfo();
	BOOL
	IsValid() {
		return (m_isValid);
	}
	DWORD
	GetStatus() { return (m_status); }

	BOOL
	CopyFileCheckVersion(CVersionInfo &originalFile);
	BOOL
	CopyFileNoVersion(CVersionInfo &originalFile);

	const CString &
	GetFilename() {
		return (m_filename);
	}

	/* Extract the elements of the file's string info block */
	CString
	GetFileVersionString();
	CString
	GetProductVersionString();
	CString
	GetComments();
	CString
	GetFileDescription();
	CString
	GetInternalName();
	CString
	GetLegalCopyright();
	CString
	GetLegalTrademarks();
	CString
	GetOriginalFileName();
	CString
	GetProductName();
	CString
	GetSpecialBuildString();
	CString
	GetPrivateBuildString();
	CString
	GetCompanyName();

	/* Extract the elements of the file's VS_FIXEDFILEINFO block */
	_int64
	GetFileVersion();
	_int64
	GetProductVersion();
	_int64
	GetFileDate();

	DWORD
	GetFileFlagMask();
	DWORD
	GetFileFlags();
	DWORD
	GetFileOS();
	DWORD
	GetFileType();
	DWORD
	GetFileSubType();

      private:
	CString m_filename;
	BOOL m_isValid;
	LPVOID m_versionInfo;
	VS_FIXEDFILEINFO *m_fixedInfo;
	DWORD m_codePage;
	DWORD m_status;

	CString
	QueryStringValue(CString value);
};

#endif /* !defined(AFX_VERSIONINFO_H__F82E9FF3_5298_11D4_AB87_00C04F789BA0__INCLUDED_) \
	*/
