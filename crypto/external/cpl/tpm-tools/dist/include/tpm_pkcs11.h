/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2005 International Business
 * Machines Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 */

#ifndef __TPM_PKCS11_H
#define __TPM_PKCS11_H

#include <tpm_utils.h>

#include <opencryptoki/pkcs11.h>

#define TPM_OPENCRYPTOKI_SO	"libopencryptoki.so"
#define TPM_TOKEN_LABEL		"IBM PKCS#11 TPM Token"
#define TPM_FIND_MAX		10

typedef int (*TokenCryptGet)( CK_BYTE  **a_pbData,
                              CK_ULONG  *a_pulDataLen,
                              CK_BBOOL  *a_pbMoreData,
                              CK_BBOOL   a_bEncrypt );

typedef int (*TokenCryptPut)( CK_BYTE  *a_pbData,
                              CK_ULONG  a_ulDataLen,
                              CK_BBOOL  a_bMoreData,
                              CK_BBOOL  a_bEncrypt );

void pkcsDebug(const char *a_pszName, CK_RV a_tResult);
void pkcsError(const char *a_pszName, CK_RV a_tResult);
void pkcsResult(const char *a_pszName, CK_RV a_tResult);
void pkcsResultException(const char *a_pszName, CK_RV a_tResult, CK_RV a_tExcept);

void pkcsSlotInfo(CK_SLOT_INFO *a_ptSlotInfo);
void pkcsTokenInfo(CK_TOKEN_INFO *a_ptTokenInfo);

CK_RV openToken( char *a_pszTokenLabel );
CK_RV closeToken( );

CK_RV initToken( char *a_pszPin );

CK_RV openTokenSession( CK_FLAGS           a_tType,
                        CK_SESSION_HANDLE *a_phSession );
CK_RV closeTokenSession( CK_SESSION_HANDLE  a_hSession );
CK_RV closeAllTokenSessions( );

CK_RV loginToken( CK_SESSION_HANDLE  a_hSession,
                  CK_USER_TYPE       a_tType,
                  char              *a_pszPin );

CK_RV initPin( CK_SESSION_HANDLE  a_hSession,
               char              *a_pszPin );
CK_RV setPin( CK_SESSION_HANDLE  a_hSession,
              char              *a_pszOldPin,
              char              *a_pszNewPin );

CK_RV generateKey( CK_SESSION_HANDLE  a_hSession,
                   CK_MECHANISM      *a_ptMechanism,
                   CK_ATTRIBUTE      *a_ptAttrList,
                   CK_ULONG           a_ulAttrCount,
                   CK_OBJECT_HANDLE  *a_phObject );

CK_RV createObject( CK_SESSION_HANDLE  a_hSession,
                    CK_ATTRIBUTE      *a_ptAttrList,
                    CK_ULONG           a_ulAttrCount,
                    CK_OBJECT_HANDLE  *a_phObject );
CK_RV destroyObject( CK_SESSION_HANDLE  a_hSession,
                     CK_OBJECT_HANDLE   a_hObject );

CK_RV getObjectAttributes( CK_SESSION_HANDLE  a_hSession,
                           CK_OBJECT_HANDLE   a_hObject,
                           CK_ATTRIBUTE      *a_ptAttrList,
                           CK_ULONG           a_ulAttrCount );

CK_RV findObjects( CK_SESSION_HANDLE  a_hSession,
                   CK_ATTRIBUTE      *a_ptAttrList,
                   CK_ULONG           a_ulAttrCount,
                   CK_OBJECT_HANDLE **a_phObjList,
                   CK_ULONG          *a_pulObjCount );

CK_RV displayObject( CK_SESSION_HANDLE  a_hSession,
                     CK_OBJECT_HANDLE   a_hObject,
                     int                a_bExtended );

CK_RV encryptData( CK_SESSION_HANDLE  a_hSession,
                   CK_OBJECT_HANDLE   a_hObject,
                   CK_MECHANISM      *a_ptMechanism,
                   TokenCryptGet      a_fGet,
                   TokenCryptPut      a_fPut );
CK_RV decryptData( CK_SESSION_HANDLE  a_hSession,
                   CK_OBJECT_HANDLE   a_hObject,
                   CK_MECHANISM      *a_ptMechanism,
                   TokenCryptGet      a_fGet,
                   TokenCryptPut      a_fPut );

BOOL isTokenInitialized( );
int  getMinPinLen( );
int  getMaxPinLen( );

#endif
