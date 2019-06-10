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

#include <tpm_pkcs11.h>

#include <stdlib.h>
#include <dlfcn.h>
#include <opencryptoki/pkcs11.h>


/*
 * Global variables
 */
char *                g_pszSoLib = TPM_OPENCRYPTOKI_SO;
void *                g_pSoLib   = NULL; // Handle of libopencryptoki library
CK_FUNCTION_LIST_PTR  g_pFcnList = NULL; // Function List

BOOL           g_bInit      = FALSE;	// Indicates if C_Initialize has been called
BOOL           g_bTokenOpen = FALSE;	// Indicates if the token has been opened
CK_SLOT_ID     g_tSlotId;		// Slot ID of the TPM token
CK_TOKEN_INFO  g_tToken;		// TPM token information


void
pkcsDebug( const char *a_pszName,
           CK_RV       a_tResult ) {

	logDebug( _("%s success\n"), a_pszName );
}

void
pkcsError( const char *a_pszName,
           CK_RV       a_tResult ) {

	logError( _("%s failed: 0x%08x (%ld)\n"), a_pszName, a_tResult, a_tResult );
}

void
pkcsResult( const char *a_pszName,
            CK_RV       a_tResult ) {

	if ( a_tResult == CKR_OK )
		pkcsDebug( a_pszName, a_tResult );
	else
		pkcsError( a_pszName, a_tResult );
}

void
pkcsResultException( const char *a_pszName,
                     CK_RV       a_tResult,
                     CK_RV       a_tExcept ) {

	if ( ( a_tResult == CKR_OK ) || ( a_tResult == a_tExcept ) )
		pkcsDebug( a_pszName, a_tResult );
	else
		pkcsError( a_pszName, a_tResult );
}

/*
 * pkcsSlotInfo
 *   Display some information about the slot.
 */
void
pkcsSlotInfo(CK_SLOT_INFO *a_ptSlotInfo ) {

	char szSlotDesc[ sizeof( a_ptSlotInfo->slotDescription ) + 1 ];
	char szSlotMfr[ sizeof( a_ptSlotInfo->manufacturerID ) + 1 ];

	__memset( szSlotDesc, 0, sizeof( szSlotDesc ) );
	__memset( szSlotMfr, 0, sizeof( szSlotMfr ) );

	strncpy( szSlotDesc, (char *)a_ptSlotInfo->slotDescription,
			sizeof( a_ptSlotInfo->slotDescription ) );
	strncpy( szSlotMfr, (char *)a_ptSlotInfo->manufacturerID,
			sizeof( a_ptSlotInfo->manufacturerID ) );

	logDebug( _("Slot description: %s\n"), szSlotDesc );
	logDebug( _("Slot manufacturer: %s\n"), szSlotMfr );
	if ( a_ptSlotInfo->flags & CKF_TOKEN_PRESENT )
		logDebug( _("Token is present\n") );
	else
		logDebug( _("Token is not present\n") );
}

/*
 * pkcsTokenInfo
 *   Display some information about the token.
 */
void
pkcsTokenInfo(CK_TOKEN_INFO *a_ptTokenInfo ) {

	char szTokenLabel[ sizeof( a_ptTokenInfo->label ) + 1 ];
	char szTokenMfr[ sizeof( a_ptTokenInfo->manufacturerID ) + 1 ];
	char szTokenModel[ sizeof( a_ptTokenInfo->model ) + 1 ];

	__memset( szTokenLabel, 0, sizeof( szTokenLabel ) );
	__memset( szTokenMfr, 0, sizeof( szTokenMfr ) );
	__memset( szTokenModel, 0, sizeof( szTokenModel ) );

	strncpy( szTokenLabel, (char *)a_ptTokenInfo->label,
			sizeof( a_ptTokenInfo->label ) );
	strncpy( szTokenMfr, (char *)a_ptTokenInfo->manufacturerID,
			sizeof( a_ptTokenInfo->manufacturerID ) );
	strncpy( szTokenModel, (char *)a_ptTokenInfo->model,
			sizeof( a_ptTokenInfo->model ) );

	logDebug( _("Token Label: %s\n"), szTokenLabel );
	logDebug( _("Token manufacturer: %s\n"), szTokenMfr );
	logDebug( _("Token model: %s\n"), szTokenModel );

	if ( a_ptTokenInfo->flags & CKF_TOKEN_INITIALIZED )
		logDebug( _("Token is initialized\n") );
	else
		logDebug( _("Token is not initialized\n") );
}

/*
 * openToken
 *   Iterate through the available slots and tokens looking
 *   for the TPM token and "opening" it if it is found.
 */
CK_RV
openToken( char *a_pszTokenLabel ) {

	CK_C_GetFunctionList  fGetFunctionList;

	unsigned int  i;

	CK_RV          rv;
	CK_ULONG       ulSlots;
	CK_SLOT_ID    *ptSlots = NULL;
	CK_SLOT_INFO   tSlotInfo;
	CK_TOKEN_INFO  tTokenInfo;

	char  szTokenLabel[ sizeof( tTokenInfo.label ) ];
	char *pszTokenLabel;

	// Load the PKCS#11 library
	g_pSoLib = dlopen( g_pszSoLib, RTLD_NOW );
	if ( !g_pSoLib ) {
		logError( _("The PKCS#11 library cannot be loaded: %s\n"), dlerror( ) );
		rv = CKR_GENERAL_ERROR;
		goto out;
	}
	fGetFunctionList = (CK_C_GetFunctionList)dlsym( g_pSoLib, "C_GetFunctionList" );
	if ( !fGetFunctionList ) {
		logError( _("Unable to find the C_GetFunctionList function: %s\n"), dlerror( ) );
		rv = CKR_GENERAL_ERROR;
		goto out;
	}
	rv = fGetFunctionList( &g_pFcnList );
	pkcsResult( "C_GetFunctionList", rv );
	if ( rv != CKR_OK )
		goto out;

	// Set the name of the TPM token
	__memset( szTokenLabel, ' ', sizeof( szTokenLabel ) );
	if ( a_pszTokenLabel ) {
		if ( strlen( a_pszTokenLabel ) > sizeof( szTokenLabel ) ) {
			logError( _("The token label cannot be greater than %ld characters\n"), sizeof( szTokenLabel ) );
			rv = CKR_GENERAL_ERROR;
			goto out;
		}
		pszTokenLabel = a_pszTokenLabel;
	}
	else
		pszTokenLabel = TPM_TOKEN_LABEL;
	strncpy( szTokenLabel, pszTokenLabel, strlen( pszTokenLabel ) );

	// Initialize the PKCS#11 library
	rv = g_pFcnList->C_Initialize( NULL );
	pkcsResult( "C_Initialize", rv );
	if ( rv != CKR_OK )
		goto out;
	g_bInit = TRUE;

	// Determine the number of slots that are present
	rv = g_pFcnList->C_GetSlotList( FALSE, NULL, &ulSlots );
	pkcsResult( "C_GetSlotList", rv );
	if ( rv != CKR_OK )
		goto out;

	if ( ulSlots == 0 ) {
		logError( _("No PKCS#11 slots present\n") );
		rv = CKR_TOKEN_NOT_PRESENT;
		goto out;
	}

	// Allocate a buffer to hold the slot ids
	logDebug( _("Slots present: %ld\n"), ulSlots );
	ptSlots = (CK_SLOT_ID_PTR)calloc( 1, sizeof( CK_SLOT_ID ) * ulSlots );
	if ( !ptSlots ) {
		logError( _("Unable to obtain memory for PKCS#11 slot IDs\n") );
		rv = CKR_HOST_MEMORY;
		goto out;
	}

	// Retrieve the list of slot ids that are present
	rv = g_pFcnList->C_GetSlotList( FALSE, ptSlots, &ulSlots );
	pkcsResult( "C_GetSlotList", rv );
	if ( rv != CKR_OK )
		goto out;

	// Iterate through the slots looking for the TPM token
	for ( i = 0; i < ulSlots; i++ ) {
		// Obtain information about the slot
		logDebug( _("Retrieving slot information for SlotID %ld\n"), ptSlots[ i ] );
		rv = g_pFcnList->C_GetSlotInfo( ptSlots[ i ], &tSlotInfo );
		pkcsResult( "C_GetSlotInfo", rv );
		if ( rv != CKR_OK )
			goto out;
		pkcsSlotInfo( &tSlotInfo );

		if ( tSlotInfo.flags & CKF_TOKEN_PRESENT ) {
			// The slot token is present, obtain information about the token
			logDebug( _("Retrieving token information for SlotID %ld\n"), ptSlots[ i ] );
			rv = g_pFcnList->C_GetTokenInfo( ptSlots[ i ], &tTokenInfo );
			pkcsResult( "C_GetTokenInfo", rv );
			if ( rv != CKR_OK )
				goto out;
			pkcsTokenInfo( &tTokenInfo );

			// Check for the TPM token
			if ( !strncmp( (char *)tTokenInfo.label, szTokenLabel, sizeof( szTokenLabel ) ) ) {
				g_bTokenOpen = TRUE;
				g_tSlotId = ptSlots[ i ];
				g_tToken = tTokenInfo;
				break;
			}
		}
	}

	if ( !g_bTokenOpen ) {
		logError( _("PKCS#11 TPM Token is not present\n") );
		rv = CKR_TOKEN_NOT_PRESENT;
	}

out:
	if (rv != CKR_OK)
		free(ptSlots);

	if ( !g_bTokenOpen && g_bInit ) {
		g_pFcnList->C_Finalize( NULL );
		g_bInit = FALSE;
	}

	if ( !g_bTokenOpen && g_pSoLib ) {
		dlclose( g_pSoLib );
		g_pSoLib = NULL;
	}

	return rv;
}

/*
 * closeToken
 *   "Close" the TPM token.
 */
CK_RV
closeToken( ) {

	CK_RV  rv = CKR_OK;

	// Tear down the PKCS#11 environment
	if ( g_bInit ) {
		rv = g_pFcnList->C_Finalize( NULL );
		pkcsResult( "C_Finalize", rv );
	}

	// Unload the PKCS#11 library
	if ( g_pSoLib )
		dlclose( g_pSoLib );

	g_bTokenOpen = FALSE;
	g_bInit      = FALSE;
	g_pSoLib     = NULL;

	return rv;
}

/*
 * initToken
 *   Invoke the PKCS#11 C_InitToken API.
 */
CK_RV
initToken( char *a_pszPin ) {

	CK_RV  rv;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	rv = g_pFcnList->C_InitToken( g_tSlotId, (CK_CHAR *)a_pszPin, strlen( a_pszPin ), g_tToken.label );
	pkcsResult( "C_InitToken", rv );

	return rv;
}

/*
 * openTokenSession
 *   Invoke the PKCS#11 C_OpenSession API.
 */
CK_RV
openTokenSession( CK_FLAGS           a_tType,
                  CK_SESSION_HANDLE *a_phSession ) {

	CK_RV  rv;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	a_tType |= CKF_SERIAL_SESSION; // This flag must always be set
	rv = g_pFcnList->C_OpenSession( g_tSlotId, a_tType, NULL, NULL, a_phSession );
	pkcsResult( "C_OpenSession", rv );

	return rv;
}

/*
 * closeTokenSession
 *   Invoke the PKCS#11 C_CloseSession API.
 */
CK_RV
closeTokenSession( CK_SESSION_HANDLE  a_hSession ) {

	CK_RV  rv;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	rv = g_pFcnList->C_CloseSession( a_hSession );
	pkcsResult( "C_CloseSession", rv );

	return rv;
}

/*
 * closeAllTokenSessions
 *   Invoke the PKCS#11 C_CloseAllSessions API.
 */
CK_RV
closeAllTokenSessions( ) {

	CK_RV  rv;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	rv = g_pFcnList->C_CloseAllSessions( g_tSlotId );
	pkcsResult( "C_CloseAllSessions", rv );

	return rv;
}

/*
 * loginToken
 *   Invoke the PKCS#11 C_Login API for the specified user type.
 */
CK_RV
loginToken( CK_SESSION_HANDLE  a_hSession,
            CK_USER_TYPE       a_tType,
            char              *a_pszPin ) {

	CK_RV  rv;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	rv = g_pFcnList->C_Login( a_hSession, a_tType, (CK_CHAR *)a_pszPin, strlen( a_pszPin ) );
	pkcsResult( "C_Login", rv );

	return rv;
}

/*
 *   Invoke the PKCS#11 C_InitPin API.
 */
CK_RV
initPin( CK_SESSION_HANDLE  a_hSession,
         char              *a_pszPin ) {

	CK_RV  rv;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	rv = g_pFcnList->C_InitPIN( a_hSession, (CK_CHAR *)a_pszPin, strlen( a_pszPin ) );
	pkcsResult( "C_InitPIN", rv );

	return rv;
}

/*
 * setPin
 *   Invoke the PKCS#11 C_SetPIN API.
 */
CK_RV
setPin( CK_SESSION_HANDLE  a_hSession,
        char              *a_pszOldPin,
        char              *a_pszNewPin ) {

	CK_RV  rv;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	rv = g_pFcnList->C_SetPIN( a_hSession, (CK_CHAR *)a_pszOldPin, strlen( a_pszOldPin ),
			(CK_CHAR *)a_pszNewPin, strlen( a_pszNewPin ) );
	pkcsResult( "C_SetPIN", rv );

	return rv;
}

/*
 * generateKey
 *   Invoke the PKCS#11 C_GenerateKey API to generate a key
 *   for the specified mechanism with the specified attributes.
 */
CK_RV
generateKey( CK_SESSION_HANDLE  a_hSession,
             CK_MECHANISM      *a_ptMechanism,
             CK_ATTRIBUTE      *a_ptAttrList,
             CK_ULONG           a_ulAttrCount,
             CK_OBJECT_HANDLE  *a_phObject ) {

	CK_RV  rv;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	rv = g_pFcnList->C_GenerateKey( a_hSession, a_ptMechanism, a_ptAttrList, a_ulAttrCount, a_phObject );
	pkcsResult( "C_GenerateKey", rv );

	return rv;
}

/*
 * createObject
 *   Invoke the PKCS#11 C_CreateObject API to create an object
 *   with the specified attributes.
 */
CK_RV
createObject( CK_SESSION_HANDLE  a_hSession,
              CK_ATTRIBUTE      *a_ptAttrList,
              CK_ULONG           a_ulAttrCount,
              CK_OBJECT_HANDLE  *a_phObject ) {

	CK_RV  rv;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	rv = g_pFcnList->C_CreateObject( a_hSession, a_ptAttrList, a_ulAttrCount, a_phObject );
	pkcsResult( "C_CreateObject", rv );

	return rv;
}

/*
 * destroyObject
 *   Invoke the PKCS#11 C_DestroyObject API.
 */
CK_RV
destroyObject( CK_SESSION_HANDLE  a_hSession,
               CK_OBJECT_HANDLE   a_hObject ) {

	CK_RV  rv;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	rv = g_pFcnList->C_DestroyObject( a_hSession, a_hObject );
	pkcsResult( "C_DestroyObject", rv );

	return rv;
}

/*
 * getObjectAttributes
 *   Invoke the PKCS#11 C_GetAttributeValue API to retrieve
 *   the specified attributes.
 */
CK_RV
getObjectAttributes( CK_SESSION_HANDLE  a_hSession,
                     CK_OBJECT_HANDLE   a_hObject,
                     CK_ATTRIBUTE      *a_ptAttrList,
                     CK_ULONG           a_ulAttrCount ) {

	CK_RV  rv;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	rv = g_pFcnList->C_GetAttributeValue( a_hSession, a_hObject, a_ptAttrList, a_ulAttrCount );
	pkcsResultException( "C_GetAttributeValue", rv, CKR_ATTRIBUTE_TYPE_INVALID );

	return rv;
}

/*
 * findObjects
 *   Return a list of object handles for all objects that
 *   match the specified attributes.
 */
CK_RV
findObjects( CK_SESSION_HANDLE  a_hSession,
             CK_ATTRIBUTE      *a_ptAttrList,
             CK_ULONG           a_ulAttrCount,
             CK_OBJECT_HANDLE **a_phObjList,
             CK_ULONG          *a_pulObjCount ) {

	CK_RV             rv, rv_temp;
	CK_ULONG          ulCount    = 0;
	CK_ULONG          ulCurCount = 0;
	CK_ULONG          ulMaxCount = 0;
	CK_OBJECT_HANDLE *phObjList  = NULL;

	*a_phObjList = NULL;
	*a_pulObjCount = 0;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	// Initialize the find operation
	rv = g_pFcnList->C_FindObjectsInit( a_hSession, a_ptAttrList, a_ulAttrCount );
	pkcsResult( "C_FindObjectsInit", rv );
	if ( rv != CKR_OK )
		goto out;

	// Iterate until all object handles have been returned
	do {
		// Allocate (or increase) the object handle list buffer
		CK_OBJECT_HANDLE *phTemp = phObjList;

		ulMaxCount += TPM_FIND_MAX;
		phObjList = (CK_OBJECT_HANDLE *)calloc( sizeof( CK_OBJECT_HANDLE ), ulMaxCount );
		if ( !phObjList ) {
			logError( _("Unable to obtain memory for object handle list\n") );
			rv = CKR_HOST_MEMORY;
			goto done;
		}

		// Copy the list of object handles
		if ( phTemp ) {
			memcpy( phObjList, phTemp, ulCurCount * sizeof( CK_OBJECT_HANDLE ) );
			free( phTemp );
		}

		// Find the matching objects
		rv = g_pFcnList->C_FindObjects( a_hSession, phObjList + ulCurCount, TPM_FIND_MAX, &ulCount );
		pkcsResult( "C_FindObjects", rv );
		if ( rv != CKR_OK )
			goto done;

		ulCurCount += ulCount;
	} while ( ulCurCount == ulMaxCount );

	*a_phObjList = phObjList;
	*a_pulObjCount = ulCurCount;

done:
	// Terminate the find operation
	rv_temp = g_pFcnList->C_FindObjectsFinal( a_hSession );
	pkcsResult( "C_FindObjectsFinal", rv_temp );

out:
	if ( ( rv != CKR_OK ) && phObjList )
		free( phObjList );

	return rv;
}

/*
 * displayByteArray
 *   Format a byte array for display.
 */
void
displayByteArray( const char   *a_pszLabel,
                  CK_ATTRIBUTE *a_ptAttr,
                  int           a_bExtended ) {

	const char *pszPre  = ( a_bExtended ) ? "\t" : "";
	const char *pszPost = ( a_bExtended ) ? "\n" : "";

	logMsg( "%s%s'", pszPre, a_pszLabel );

	if ( a_ptAttr->ulValueLen )
		logHex( a_ptAttr->ulValueLen, a_ptAttr->pValue );
	else
		logMsg( "(null)" );

	logMsg( "'%s", pszPost );
}

/*
 * displayCertObject
 *   Format a certificate object for display.
 */
CK_RV
displayCertObject( CK_SESSION_HANDLE  a_hSession,
                   CK_OBJECT_HANDLE   a_hObject,
                   int                a_bExtended ) {

	CK_RV                rv;
	CK_OBJECT_CLASS  tClass;
	CK_BBOOL             bToken;
	CK_BBOOL             bPrivate;
	CK_BBOOL             bModifiable;
	CK_CHAR             *pszLabel    = NULL;
	CK_CERTIFICATE_TYPE  tType;
	CK_BBOOL             bTrusted;

	CK_ATTRIBUTE  tCertList[] = {
			{ CKA_CLASS, &tClass, sizeof( tClass ) },
			{ CKA_TOKEN, &bToken, sizeof( bToken ) },
			{ CKA_PRIVATE, &bPrivate, sizeof( bPrivate ) },
			{ CKA_MODIFIABLE, &bModifiable, sizeof( bModifiable ) },
			{ CKA_LABEL, NULL, 0 },
			{ CKA_CERTIFICATE_TYPE, &tType, sizeof( tType ) },
			{ CKA_TRUSTED, &bTrusted, sizeof( bTrusted ) },
		};
	CK_ATTRIBUTE  tX509List[] = {
			{ CKA_SUBJECT, NULL, 0 },
			{ CKA_ID, NULL, 0 },
			{ CKA_ISSUER, NULL, 0 },
			{ CKA_SERIAL_NUMBER, NULL, 0 },
			{ CKA_VALUE, NULL, 0 },
		};
	CK_ATTRIBUTE  tX509AttrList[] = {
			{ CKA_OWNER, NULL, 0 },
			{ CKA_AC_ISSUER, NULL, 0 },
			{ CKA_SERIAL_NUMBER, NULL, 0 },
			{ CKA_ATTR_TYPES, NULL, 0 },
			{ CKA_VALUE, NULL, 0 },
		};
	CK_ULONG      ulCertCount     = sizeof( tCertList ) / sizeof( CK_ATTRIBUTE );
	CK_ULONG      ulX509Count     = sizeof( tX509List ) / sizeof( CK_ATTRIBUTE );
	CK_ULONG      ulX509AttrCount = sizeof( tX509AttrList ) / sizeof( CK_ATTRIBUTE );
	CK_ATTRIBUTE *ptAttrList;
	CK_ULONG      ulAttrCount;

	// Retrieve the common certificate attributes
	rv = getObjectAttributes( a_hSession, a_hObject, tCertList, ulCertCount );
	if ( ( rv != CKR_OK ) && ( rv != CKR_ATTRIBUTE_TYPE_INVALID ) )
		return rv;

	// Allocate storage for the object label (extra byte for null
	// terminated string)
	if ( tCertList[ 4 ].ulValueLen > 0 ) {
		pszLabel = tCertList[ 4 ].pValue = calloc( 1, tCertList[ 4 ].ulValueLen + 1 );

		rv = getObjectAttributes( a_hSession, a_hObject, tCertList, ulCertCount );
		if ( ( rv != CKR_OK ) && ( rv != CKR_ATTRIBUTE_TYPE_INVALID ) )
			return rv;
	}

	// Determine the attributes to retrieve based on the certficate type
	switch ( tType ) {
		case CKC_X_509:
			ptAttrList = tX509List;
			ulAttrCount = ulX509Count;
			break;

		case CKC_X_509_ATTR_CERT:
			ptAttrList = tX509AttrList;
			ulAttrCount = ulX509AttrCount;
			break;

		default:
			ptAttrList = NULL;
			ulAttrCount = 0;
	}

	if ( ptAttrList ) {
		CK_ULONG  ulMalloc;

		// Retrieve the specific certificate type attributes (for obtaining
		// the attribute lengths)
		rv = getObjectAttributes( a_hSession, a_hObject, ptAttrList, ulAttrCount );
		if ( ( rv != CKR_OK ) && ( rv != CKR_ATTRIBUTE_TYPE_INVALID ) )
			return rv;

		for ( ulMalloc = 0; ulMalloc < ulAttrCount; ulMalloc++ ) {
			// Allocate the storage (with an extra byte for null terminated
			// strings - just in case)
			if ( ptAttrList[ ulMalloc ].ulValueLen > 0 )
				ptAttrList[ ulMalloc ].pValue =
					calloc( 1, ptAttrList[ ulMalloc ].ulValueLen );
		}

		// Now retrieve all the specific certificate type attributes
		rv = getObjectAttributes( a_hSession, a_hObject, ptAttrList, ulAttrCount );
		if ( ( rv != CKR_OK ) && ( rv != CKR_ATTRIBUTE_TYPE_INVALID ) )
			return rv;
	}

	if ( a_bExtended ) {
		logMsg( _("Certificate Object\n") );
		switch ( tType ) {
			case CKC_X_509:
				logMsg( _("\tX509 Certificate\n") );
				break;

			case CKC_X_509_ATTR_CERT:
				logMsg( _("\tX509 Attribute Certificate\n") );
				break;

			default:
				logMsg( _("\tUnknown Certificate Type (%08x)\n"), tType );
		}
		if ( tCertList[ 1 ].ulValueLen > 0 )
			logMsg( _("\tToken Object: %s\n"), bToken ? _("true") : _("false") );
		if ( tCertList[ 2 ].ulValueLen > 0 )
			logMsg( _("\tPrivate Object: %s\n"), bPrivate ? _("true") : _("false") );
		if ( tCertList[ 3 ].ulValueLen > 0 )
			logMsg( _("\tModifiable Object: %s\n"), bModifiable ? _("true") : _("false") );
		if ( tCertList[ 4 ].ulValueLen > 0 )
			logMsg( _("\tLabel: '%s'\n"), pszLabel );
		if ( tCertList[ 5 ].ulValueLen > 0 )
			logMsg( _("\tTrusted: %s\n"), bTrusted ? _("true") : _("false") );

		// Display the attributes based on the certficate type
		switch ( tType ) {
			case CKC_X_509:
				if ( tX509List[ 0 ].ulValueLen > 0 )
					displayByteArray( _("Subject: "), &tX509List[ 0 ], a_bExtended );
				if ( tX509List[ 1 ].ulValueLen > 0 ) {
					logMsg( _("\tId: '%s' ("), tX509List[ 1 ].pValue );
					displayByteArray( "", &tX509List[ 1 ], FALSE );
					logMsg( ")\n" );
				}
				if ( tX509List[ 2 ].ulValueLen > 0 )
					displayByteArray( _("Issuer: "), &tX509List[ 2 ], a_bExtended );
				if ( tX509List[ 3 ].ulValueLen > 0 )
					displayByteArray( _("Serial Number: "), &tX509List[ 3 ], a_bExtended );
				if ( tX509List[ 4 ].ulValueLen > 0 )
					displayByteArray( _("Value: "), &tX509List[ 4 ], a_bExtended );
				break;

			case CKC_X_509_ATTR_CERT:
				if ( tX509AttrList[ 0 ].ulValueLen > 0 )
					displayByteArray( _("Owner: "), &tX509AttrList[ 0 ], a_bExtended );
				if ( tX509AttrList[ 1 ].ulValueLen > 0 )
					displayByteArray( _("Issuer: "), &tX509AttrList[ 1 ], a_bExtended );
				if ( tX509AttrList[ 2 ].ulValueLen > 0 )
					displayByteArray( _("Serial Number: "), &tX509AttrList[ 2 ], a_bExtended );
				if ( tX509AttrList[ 3 ].ulValueLen > 0 )
					displayByteArray( _("Attribute Types: "), &tX509AttrList[ 3 ], a_bExtended );
				if ( tX509AttrList[ 4 ].ulValueLen > 0 )
					displayByteArray( _("Value: "), &tX509AttrList[ 4 ], a_bExtended );
				break;
		}
	}
	else {
		// Display the attributes based on the certficate type
		logMsg( _("Certificate: ") );
		switch ( tType ) {
			case CKC_X_509:
				logMsg( _("Type: X509 Public Key") );
				break;

			case CKC_X_509_ATTR_CERT:
				logMsg( _("Type: X509 Attribute") );
				break;

			default:
				logMsg( _("Unknown Type (%08x)"), tType );
		}

		if ( tCertList[ 4 ].ulValueLen > 0 )
			logMsg( _(", Label: '%s'"), pszLabel );

		logMsg( "\n" );
	}

	return rv;
}

/*
 * displayAsymKeyObject
 *   Format an asymmetric key object for display.
 */
CK_RV
displayAsymKeyObject( CK_SESSION_HANDLE  a_hSession,
                      CK_OBJECT_HANDLE   a_hObject,
                      int                a_bExtended ) {

	CK_RV            rv;
	CK_OBJECT_CLASS  tClass;
	CK_BBOOL         bToken;
	CK_BBOOL         bPrivate;
	CK_BBOOL         bModifiable;
	CK_CHAR         *pszLabel     = NULL;
	CK_KEY_TYPE      tType;
	CK_CHAR         *pszId        = NULL;

	CK_ATTRIBUTE  tKeyList[] = {
			{ CKA_CLASS, &tClass, sizeof( tClass ) },
			{ CKA_TOKEN, &bToken, sizeof( bToken ) },
			{ CKA_PRIVATE, &bPrivate, sizeof( bPrivate ) },
			{ CKA_MODIFIABLE, &bModifiable, sizeof( bModifiable ) },
			{ CKA_LABEL, NULL, 0 },
			{ CKA_KEY_TYPE, &tType, sizeof( tType ) },
			{ CKA_SUBJECT, NULL, 0 },
			{ CKA_ID, NULL, 0 },
		};
	CK_ULONG      ulKeyCount = sizeof( tKeyList ) / sizeof( CK_ATTRIBUTE );

	// Retrieve the common key attributes
	rv = getObjectAttributes( a_hSession, a_hObject, tKeyList, ulKeyCount );
	if ( ( rv != CKR_OK ) && ( rv != CKR_ATTRIBUTE_TYPE_INVALID ) )
		return rv;

	// Allocate storage for the object id
	if ( ( tKeyList[ 4 ].ulValueLen > 0 ) || ( tKeyList[ 6 ].ulValueLen > 0 ) || ( tKeyList[ 7 ].ulValueLen > 0 ) ) {
		if ( tKeyList[ 4 ].ulValueLen > 0 )
			pszLabel = tKeyList[ 4 ].pValue =
				calloc( 1, tKeyList[ 4 ].ulValueLen + 1 );

		if ( tKeyList[ 6 ].ulValueLen > 0 )
			tKeyList[ 6 ].pValue =
				calloc( 1, tKeyList[ 6 ].ulValueLen + 1 );

		if ( tKeyList[ 7 ].ulValueLen > 0 )
			pszId = tKeyList[ 7 ].pValue =
				calloc( 1, tKeyList[ 7 ].ulValueLen + 1 );

		rv = getObjectAttributes( a_hSession, a_hObject, tKeyList, ulKeyCount );
		if ( ( rv != CKR_OK ) && ( rv != CKR_ATTRIBUTE_TYPE_INVALID ) )
			return rv;
	}

	if ( a_bExtended ) {
		logMsg( _("Key Object\n") );
		switch ( tClass ) {
			case CKO_PUBLIC_KEY:
				logMsg( _("\tPublic Key\n") );
				break;

			case CKO_PRIVATE_KEY:
				logMsg( _("\tPrivate Key\n") );
				break;
		}
		if ( tKeyList[ 1 ].ulValueLen > 0 )
			logMsg( _("\tToken Object: %s\n"), bToken ? _("true") : _("false") );
		if ( tKeyList[ 2 ].ulValueLen > 0 )
			logMsg( _("\tPrivate Object: %s\n"), bPrivate ? _("true") : _("false") );
		if ( tKeyList[ 3 ].ulValueLen > 0 )
			logMsg( _("\tModifiable Object: %s\n"), bModifiable ? _("true") : _("false") );
		if ( tKeyList[ 4 ].ulValueLen > 0 )
			logMsg( _("\tLabel: '%s'\n"), pszLabel );
		if ( tKeyList[ 5 ].ulValueLen > 0 )
			logMsg( _("\tType: %ld\n"), tType );
		if ( tKeyList[ 6 ].ulValueLen > 0 )
			displayByteArray( _("Subject: "), &tKeyList[ 6 ], a_bExtended );
		if ( tKeyList[ 7 ].ulValueLen > 0 ) {
			logMsg( _("\tId: '%s' ("), pszId );
			displayByteArray( "", &tKeyList[ 7 ], FALSE );
			logMsg( ")\n" );
		}
	}
	else {
		switch ( tClass ) {
			case CKO_PUBLIC_KEY:
				logMsg( _("Public Key: ") );
				break;

			case CKO_PRIVATE_KEY:
				logMsg( _("Private Key: ") );
				break;
		}

		if ( tKeyList[ 5 ].ulValueLen > 0 )
			logMsg( _("Type: %ld"), tType );
		if ( tKeyList[ 4 ].ulValueLen > 0 )
			logMsg( _(", Label: '%s'"), pszLabel );

		logMsg( "\n" );
	}

	return rv;
}

/*
 * displaySymKeyObject
 *   Format a symmetric key object for display.
 */
CK_RV
displaySymKeyObject( CK_SESSION_HANDLE  a_hSession,
                     CK_OBJECT_HANDLE   a_hObject,
                     int                a_bExtended ) {

	CK_RV            rv;
	CK_OBJECT_CLASS  tClass;
	CK_BBOOL         bToken;
	CK_BBOOL         bPrivate;
	CK_BBOOL         bModifiable;
	CK_CHAR         *pszLabel     = NULL;
	CK_KEY_TYPE      tType;

	CK_ATTRIBUTE  tKeyList[] = {
			{ CKA_CLASS, &tClass, sizeof( tClass ) },
			{ CKA_TOKEN, &bToken, sizeof( bToken ) },
			{ CKA_PRIVATE, &bPrivate, sizeof( bPrivate ) },
			{ CKA_MODIFIABLE, &bModifiable, sizeof( bModifiable ) },
			{ CKA_LABEL, NULL, 0 },
			{ CKA_KEY_TYPE, &tType, sizeof( tType ) },
		};
	CK_ULONG      ulKeyCount = sizeof( tKeyList ) / sizeof( CK_ATTRIBUTE );

	// Retrieve the common key attributes
	rv = getObjectAttributes( a_hSession, a_hObject, tKeyList, ulKeyCount );
	if ( ( rv != CKR_OK ) && ( rv != CKR_ATTRIBUTE_TYPE_INVALID ) )
		return rv;

	// Allocate storage for the object id
	if ( tKeyList[ 4 ].ulValueLen > 0 ) {
		pszLabel = tKeyList[ 4 ].pValue =
			calloc( 1, tKeyList[ 4 ].ulValueLen + 1 );

		rv = getObjectAttributes( a_hSession, a_hObject, tKeyList, ulKeyCount );
		if ( ( rv != CKR_OK ) && ( rv != CKR_ATTRIBUTE_TYPE_INVALID ) )
			return rv;
	}

	if ( a_bExtended ) {
		logMsg( _("Key Object\n") );
		switch ( tClass ) {
			case CKO_SECRET_KEY:
				logMsg( _("\tSecret Key\n") );
				break;
		}
		if ( tKeyList[ 1 ].ulValueLen > 0 )
			logMsg( _("\tToken Object: %s\n"), bToken ? _("true") : _("false") );
		if ( tKeyList[ 2 ].ulValueLen > 0 )
			logMsg( _("\tPrivate Object: %s\n"), bPrivate ? _("true") : _("false") );
		if ( tKeyList[ 3 ].ulValueLen > 0 )
			logMsg( _("\tModifiable Object: %s\n"), bModifiable ? _("true") : _("false") );
		if ( tKeyList[ 4 ].ulValueLen > 0 )
			logMsg( _("\tLabel: '%s'\n"), pszLabel );
		if ( tKeyList[ 5 ].ulValueLen > 0 )
			logMsg( _("\tType: %ld\n"), tType );
	}
	else {
		switch ( tClass ) {
			case CKO_SECRET_KEY:
				logMsg( _("Secret Key: ") );
				break;
		}

		if ( tKeyList[ 5 ].ulValueLen > 0 )
			logMsg( _("Type: %ld"), tType );
		if ( tKeyList[ 4 ].ulValueLen > 0 )
			logMsg( _(", Label: '%s'"), pszLabel );

		logMsg( "\n" );
	}

	return rv;
}

/*
 * displayObject
 *   Format and display objects.
 */
CK_RV
displayObject( CK_SESSION_HANDLE  a_hSession,
               CK_OBJECT_HANDLE   a_hObject,
	       int                a_bExtended ) {

	CK_RV            rv;
	CK_OBJECT_CLASS  tClass;
	CK_ATTRIBUTE     tAttr[] = {
			{ CKA_CLASS, &tClass, sizeof( tClass ) },
		};
	CK_ULONG         ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );

	// Retrieve the class attribute of the object
	rv = getObjectAttributes( a_hSession, a_hObject, tAttr, ulAttrCount );
	if ( rv != CKR_OK )
		return rv;

	// Use the object class to determine how to format it for display
	switch ( tClass ) {
		case CKO_DATA:
			logMsg( _("Data object\n") );
			break;

		case CKO_CERTIFICATE:
			displayCertObject( a_hSession, a_hObject, a_bExtended );
			break;

		case CKO_PUBLIC_KEY:
		case CKO_PRIVATE_KEY:
			displayAsymKeyObject( a_hSession, a_hObject, a_bExtended );
			break;

		case CKO_SECRET_KEY:
			displaySymKeyObject( a_hSession, a_hObject, a_bExtended );
			break;

		case CKO_HW_FEATURE:
		case CKO_DOMAIN_PARAMETERS:
		default:
			logMsg( _("Object class=%ld\n"), tClass );
			break;
	}

	return rv;
}

/*
 * checkKey
 *   Check that the key object attributes match the key class
 *   and key type specified.
 */
CK_RV
checkKey( CK_SESSION_HANDLE  a_hSession,
          CK_OBJECT_HANDLE   a_hObject,
          CK_OBJECT_CLASS    a_tKeyClass,
          CK_KEY_TYPE        a_tKeyType ) {

	CK_RV  rv;

	CK_OBJECT_CLASS  tClass;
	CK_KEY_TYPE      tType;
	CK_ATTRIBUTE     tAttr[] = {
			{ CKA_CLASS, &tClass, sizeof( tClass ) },
			{ CKA_KEY_TYPE, &tType, sizeof( tType ) },
		};
	CK_ULONG         ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );

	// Retrieve the class attribute and key type attribute of the object
	rv = getObjectAttributes( a_hSession, a_hObject, tAttr, ulAttrCount );
	if ( rv != CKR_OK )
		return rv;

	if ( tClass != a_tKeyClass )
		return CKR_GENERAL_ERROR;

	if ( tType != a_tKeyType )
		return CKR_GENERAL_ERROR;

	return CKR_OK;
}

/*
 * encryptData
 *   Use a callback mechanism to encrypt some data.
 */
CK_RV
encryptData( CK_SESSION_HANDLE  a_hSession,
             CK_OBJECT_HANDLE   a_hObject,
             CK_MECHANISM      *a_ptMechanism,
             TokenCryptGet      a_fGet,
             TokenCryptPut      a_fPut ) {

	CK_RV         rv;
	CK_BBOOL      bCancel       = FALSE;

	CK_BYTE      *pbInData      = NULL;
	CK_ULONG      ulInDataLen   = 0;
	CK_BBOOL      bContinue     = TRUE;

	CK_BYTE      *pbBuffer      = NULL;
	CK_ULONG      ulBufferLen   = 0;
	CK_ULONG      ulOutDataLen  = 0;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	// Check the key
	rv = checkKey( a_hSession, a_hObject, CKO_SECRET_KEY, CKK_AES );
	if ( rv != CKR_OK )
		goto out;

	// Initialize the encryption operation
	rv = g_pFcnList->C_EncryptInit( a_hSession, a_ptMechanism, a_hObject );
	pkcsResult( "C_EncryptInit", rv );
	if ( rv != CKR_OK )
		goto out;

	while ( bContinue ) {
		// Retrieve some data to encrypt
		if ( a_fGet( &pbInData, &ulInDataLen, &bContinue, TRUE ) == -1 ) {
			bCancel = TRUE;
			goto out;
		}

		// Check the output buffer size needed
		rv = g_pFcnList->C_EncryptUpdate( a_hSession, pbInData, ulInDataLen,
			NULL, &ulOutDataLen );
		pkcsResult( "C_EncryptUpdate", rv );
		if ( rv != CKR_OK )
			goto out;

		// Check if a larger buffer is needed
		if ( ulOutDataLen > ulBufferLen ) {
			free( pbBuffer );
			ulBufferLen = ulOutDataLen;
			pbBuffer = calloc( 1, ulBufferLen );
			if ( !pbBuffer ) {
				logError( _("Unable to obtain memory for the encrypted data buffer\n") );
				rv = CKR_HOST_MEMORY;
				goto out;
			}
		}

		// Encrypt the input data
		rv = g_pFcnList->C_EncryptUpdate( a_hSession, pbInData, ulInDataLen,
			pbBuffer, &ulOutDataLen );
		pkcsResult( "C_EncryptUpdate", rv );
		if ( rv != CKR_OK )
			goto out;

		if ( ulOutDataLen > 0 ) {
			if ( a_fPut( pbBuffer, ulOutDataLen, bContinue, TRUE ) == -1 ) {
				bCancel = TRUE;
				goto out;
			}
		}
	}

out:
	// For AES any remaining data will cause an error, so provide
	// a buffer which will not be filled in anyway
	ulOutDataLen = ulBufferLen;
	rv = g_pFcnList->C_EncryptFinal( a_hSession, pbBuffer, &ulOutDataLen );
	pkcsResult( "C_EncryptFinal", rv );

	free( pbBuffer );

	if ( bCancel )
		rv = CKR_FUNCTION_CANCELED;

	return rv;
}

/*
 * decryptData
 *   Use a callback mechanism to decrypt some data.
 */
CK_RV
decryptData( CK_SESSION_HANDLE  a_hSession,
             CK_OBJECT_HANDLE   a_hObject,
             CK_MECHANISM      *a_ptMechanism,
             TokenCryptGet      a_fGet,
             TokenCryptPut      a_fPut ) {

	CK_RV         rv;
	CK_BBOOL      bCancel       = FALSE;

	CK_BYTE      *pbInData      = NULL;
	CK_ULONG      ulInDataLen   = 0;
	CK_BBOOL      bContinue     = TRUE;

	CK_BYTE      *pbBuffer      = NULL;
	CK_ULONG      ulBufferLen   = 0;
	CK_ULONG      ulOutDataLen  = 0;

	if ( !g_bTokenOpen )
		return CKR_GENERAL_ERROR;

	// Check the key
	rv = checkKey( a_hSession, a_hObject, CKO_SECRET_KEY, CKK_AES );
	if ( rv != CKR_OK )
		goto out;

	// Initialize the decryption operation
	rv = g_pFcnList->C_DecryptInit( a_hSession, a_ptMechanism, a_hObject );
	pkcsResult( "C_DecryptInit", rv );
	if ( rv != CKR_OK )
		goto out;

	while ( bContinue ) {
		// Retrieve some data to encrypt
		if ( a_fGet( &pbInData, &ulInDataLen, &bContinue, FALSE ) == -1 ) {
			bCancel = TRUE;
			goto out;
		}

		// Check the output buffer size needed
		rv = g_pFcnList->C_DecryptUpdate( a_hSession, pbInData, ulInDataLen,
			NULL, &ulOutDataLen );
		pkcsResult( "C_DecryptUpdate", rv );
		if ( rv != CKR_OK )
			goto out;

		// Check if a larger buffer is needed
		if ( ulOutDataLen > ulBufferLen ) {
			free( pbBuffer );
			ulBufferLen = ulOutDataLen;
			pbBuffer = calloc( 1, ulBufferLen );
			if ( !pbBuffer ) {
				logError( _("Unable to obtain memory for the encrypted data buffer\n") );
				rv = CKR_HOST_MEMORY;
				goto out;
			}
		}

		// Decrypt the input data
		rv = g_pFcnList->C_DecryptUpdate( a_hSession, pbInData, ulInDataLen,
			pbBuffer, &ulOutDataLen );
		pkcsResult( "C_DecryptUpdate", rv );
		if ( rv != CKR_OK )
			goto out;

		if ( a_fPut( pbBuffer, ulOutDataLen, bContinue, FALSE ) == -1 ) {
			bCancel = TRUE;
			goto out;
		}
	}

out:
	// For AES any remaining data will cause an error, so provide
	// a buffer which will not be filled in anyway
	rv = g_pFcnList->C_DecryptFinal( a_hSession, pbBuffer, &ulOutDataLen );
	pkcsResult( "C_DecryptFinal", rv );

	free( pbBuffer );

	if ( bCancel )
		rv = CKR_FUNCTION_CANCELED;

	return rv;
}

/*
 * isTokenInitialized
 *   Returns an indicator as to whether the TPM token has been initialized.
 */
BOOL
isTokenInitialized( ) {

	if ( g_bTokenOpen && ( g_tToken.flags & CKF_TOKEN_INITIALIZED ) )
		return TRUE;

	return FALSE;
}

/*
 * getMinPinLen
 *   Returns the the minimum PIN length that the TPM token accepts.
 */
int
getMinPinLen( ) {

	if ( !g_bTokenOpen )
		return 0;

	return (int)g_tToken.ulMinPinLen;
}

/*
 * getMaxPinLen
 *   Returns the the maximum PIN length that the TPM token accepts.
 */
int
getMaxPinLen( ) {

	if ( !g_bTokenOpen )
		return 0;

	return (int)g_tToken.ulMaxPinLen;
}
