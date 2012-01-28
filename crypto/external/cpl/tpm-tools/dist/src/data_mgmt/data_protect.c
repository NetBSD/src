/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2005, 2006 International Business
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

#include "data_protect.h"
#include "data_common.h"

#include <tpm_pkcs11.h>
#include <tpm_utils.h>

#include <stdlib.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <errno.h>


/*
 * Global variables
 */
int       g_bEncrypt     = TRUE;	// Encrypt/Decrypt operation specifier

char     *g_pszInFile    = NULL;	// Input file name
FILE     *g_pInFile      = NULL;	// Input file stream
CK_BYTE  *g_pbInData     = NULL;	// Input file buffer

char     *g_pszOutFile   = NULL;	// Output file name
FILE     *g_pOutFile     = NULL;	// Output file stream
CK_ULONG  g_ulOutBuffLen = 0;		// Output file buffer length
CK_BYTE  *g_pbOutData    = NULL;	// Output file buffer
CK_ULONG  g_ulOutDataLen = 0;		// Length of data contained in output buffer

char     *g_pszToken     = NULL;	// Token label to be used

/*
 * parseCallback
 *   Process the command specific options.
 */
int
parseCallback( int         a_iOpt,
               const char *a_pszOptArg ) {

	switch ( a_iOpt ) {
		case 'd':
			g_bEncrypt = FALSE;
			break;

		case 'e':
			g_bEncrypt = TRUE;
			break;

		case 'i':
			if ( !a_pszOptArg )
				return -1;

			g_pszInFile = strdup( a_pszOptArg );
			break;

		// Use the specified token label when finding the token
		case 'k':
			if ( !a_pszOptArg )
				return -1;

			g_pszToken = strdup( a_pszOptArg );
			break;

		case 'o':
			if ( !a_pszOptArg )
				return -1;

			g_pszOutFile = strdup( a_pszOptArg );
			break;
	}

	return 0;
}

/*
 * usageCallback
 *   Display command usage information.
 */
void
usageCallback( const char *a_szCmd ) {

	logCmdHelp( a_szCmd );
	logCmdOption( "-d, --decrypt",
			_("Decrypt the input data") );
	logCmdOption( "-e, --encrypt",
			_("Encrypt the input data (default)") );
	logCmdOption( "-i, --infile FILE",
			_("Use FILE as the input to the specified operation") );
	logCmdOption( "-k, --token STRING",
			_("Use STRING to identify the label of the PKCS#11 token to be used") );
	logCmdOption( "-o, --outfile FILE",
			_("Use FILE as the output of the specified operation") );
}

/*
 * parseCmd
 *   Parse the command line options.
 */
int
parseCmd( int    a_iArgc,
          char **a_szArgv ) {

	int  rc;

	char          *szShortOpts = "dei:k:o:";
	struct option  stLongOpts[]  = {
			{ "decrypt", no_argument, NULL, 'd' },
			{ "encrypt", no_argument, NULL, 'e' },
			{ "infile", required_argument, NULL, 'i' },
			{ "token", required_argument, NULL, 'k' },
			{ "outfile", required_argument, NULL, 'o' },
		};
	int  iNumLongOpts = sizeof( stLongOpts ) / sizeof( struct option );

	rc = genericOptHandler( a_iArgc, a_szArgv,
		szShortOpts, stLongOpts, iNumLongOpts,
		parseCallback, usageCallback );
	if ( rc == -1 )
		return -1;

	// Make sure "-i" is specified until stdin support is added
	if ( !g_pszInFile ) {
		logMsg( TOKEN_INPUT_FILE_ERROR );
		rc = -1;
	}

	// Make sure "-o" is specified until stdout support is added
	if ( !g_pszOutFile ) {
		logMsg( TOKEN_OUTPUT_FILE_ERROR );
		rc = -1;
	}

	if ( rc == -1 ) {
		usageCallback( a_szArgv[ 0 ] );
		return -1;
	}

	return 0;
}

/*
 * makeKey
 *   Make the 256-bit AES symmetric key used to encrypt
 *   or decrypt the input data.
 */
int
makeKey( CK_SESSION_HANDLE  a_hSession ) {

	int  rc = -1;

	// Generate a 256-bit AES key
	CK_RV             rv;
	CK_BBOOL          bTrue       = TRUE;
	CK_BBOOL          bFalse      = FALSE;
	CK_OBJECT_CLASS   tKeyClass   = CKO_SECRET_KEY;
	CK_KEY_TYPE       tKeyType    = CKK_AES;
	CK_ULONG          ulKeyLen    = 32;
	CK_MECHANISM      tMechanism  = { CKM_AES_KEY_GEN, NULL, 0 };
	CK_ATTRIBUTE      tAttr[]     = {
			{ CKA_CLASS, &tKeyClass, sizeof( tKeyClass ) },
			{ CKA_TOKEN, &bTrue, sizeof( bTrue ) },
			{ CKA_PRIVATE, &bTrue, sizeof( bTrue ) },
			{ CKA_MODIFIABLE, &bFalse, sizeof( bFalse ) },
			{ CKA_LABEL, TOKEN_PROTECT_KEY_LABEL, strlen( TOKEN_PROTECT_KEY_LABEL ) },
			{ CKA_KEY_TYPE, &tKeyType, sizeof( tKeyType ) },
			{ CKA_SENSITIVE, &bTrue, sizeof( bTrue ) },
			{ CKA_ENCRYPT, &bTrue, sizeof( bTrue ) },
			{ CKA_DECRYPT, &bTrue, sizeof( bTrue ) },
			{ CKA_SIGN, &bFalse, sizeof( bFalse ) },
			{ CKA_VERIFY, &bFalse, sizeof( bFalse ) },
			{ CKA_WRAP, &bTrue, sizeof( bTrue ) },
			{ CKA_UNWRAP, &bTrue, sizeof( bTrue ) },
			{ CKA_EXTRACTABLE, &bFalse, sizeof( bFalse ) },
			{ CKA_VALUE_LEN, &ulKeyLen, sizeof( ulKeyLen ) },
		};
	CK_ULONG          ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );
	CK_OBJECT_HANDLE  hObject;

	// Generate the key on the token
	rv = generateKey( a_hSession, &tMechanism, tAttr, ulAttrCount, &hObject );
	if ( rv != CKR_OK )
		goto out;

	rc = 0;
out:
	return rc;
}

/*
 * getKey
 *   Get the symmetric key used for encryption or decryption.
 */
int
getKey( CK_SESSION_HANDLE  a_hSession,
        CK_OBJECT_HANDLE  *a_phObject ) {

	int  rc = -1;

	CK_RV             rv;
	CK_BBOOL          bTrue       = TRUE;
	CK_OBJECT_CLASS   tKeyClass   = CKO_SECRET_KEY;
	CK_ATTRIBUTE      tAttr[]     = {
			{ CKA_CLASS, &tKeyClass, sizeof( tKeyClass ) },
			{ CKA_TOKEN, &bTrue, sizeof( bTrue ) },
			{ CKA_LABEL, TOKEN_PROTECT_KEY_LABEL, strlen( TOKEN_PROTECT_KEY_LABEL ) },
		};
	CK_ULONG          ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );
	CK_OBJECT_HANDLE *phObjList   = NULL;
	CK_ULONG          ulObjCount  = 0;

	*a_phObject = 0;

	// Search for the protection key
	rv = findObjects( a_hSession, tAttr, ulAttrCount, &phObjList, &ulObjCount );
	if ( rv != CKR_OK )
		goto out;

	if ( ulObjCount == 0 ) {
		// Key doesn't exist, create it
		if ( makeKey( a_hSession ) == -1 )
			goto out;

		// Search for the protection key again
		rv = findObjects( a_hSession, tAttr, ulAttrCount, &phObjList, &ulObjCount );
		if ( rv != CKR_OK )
			goto out;

	}

	// Make sure we found it
	if ( ulObjCount == 0 ) {
		logError( TOKEN_NO_KEY_ERROR );
		goto out;
	}

	// Return the handle to the key
	*a_phObject = phObjList[ 0 ];

	rc = 0;

out:
	return rc;
}

/*
 * readData
 *   Callback routine that reads the input data for the encryption
 *   or decryption operation.  The operation (encryption or decryption)
 *   determines some of the logic in this routine.
 */
int
readData( CK_BYTE  **a_pbData,
          CK_ULONG  *a_pulDataLen,
          CK_BBOOL  *a_pbMoreData,
          CK_BBOOL   a_bEncrypt ) {

	CK_ULONG  iBytes;
	CK_BBOOL  bMoreData = TRUE;

	if ( !g_pInFile ) {
		// Open the input file
		errno = 0;
		g_pInFile = fopen( g_pszInFile, "r" );
		if ( !g_pInFile ) {
			logError( TOKEN_FILE_OPEN_ERROR, g_pszInFile, strerror( errno ) );
			return -1;
		}

		// Allocate an input buffer
		g_pbInData = malloc( TOKEN_BUFFER_SIZE );
		if ( !g_pbInData ) {
			logError( TOKEN_MEMORY_ERROR );
			return -1;
		}
	}

	// Read the data
	iBytes = fread( g_pbInData, 1, TOKEN_BUFFER_SIZE, g_pInFile );
	if ( feof( g_pInFile ) ) {
		fclose( g_pInFile );

		// End of file encountered, indicate that there is
		// no more data
		bMoreData = FALSE;
	}
	else {
		// Not end of file so make sure the buffer was filled
		if ( iBytes != TOKEN_BUFFER_SIZE ) {
			// Error encountered, terminate
			fclose( g_pInFile );
			return -1;
		}
	}

	if ( !bMoreData && a_bEncrypt ) {
		// No more data, so if we are encrypting then we MUST add padding
		int  iCount   = iBytes - 1;
		int  iPadding = TOKEN_AES_BLOCKSIZE - ( iBytes % TOKEN_AES_BLOCKSIZE );

		iBytes += iPadding;

		g_pbInData[iCount + iPadding] = iPadding;
		iPadding--;
		while ( iPadding > 0 ) {
			g_pbInData[iCount + iPadding] = 0;
			iPadding--;
		}
	}

	*a_pbData = g_pbInData;
	*a_pulDataLen = iBytes;
	*a_pbMoreData = bMoreData;

	return 0;
}

/*
 * writeData
 *   Callback routine that writes the output data for the encryption
 *   or decryption operation.  The operation (encryption or decryption)
 *   determines some of the logic in this routine.
 */
int
writeData( CK_BYTE  *a_pbData,
           CK_ULONG  a_ulDataLen,
           CK_BBOOL  a_bMoreData,
           CK_BBOOL  a_bEncrypt ) {

	size_t tWriteCount;

	if ( !g_pOutFile ) {
		// Open the output file
		errno = 0;
		g_pOutFile = fopen( g_pszOutFile, "w" );
		if ( !g_pOutFile ) {
			logError( TOKEN_FILE_OPEN_ERROR, g_pszOutFile, strerror( errno ) );
			return -1;
		}
	}

	if ( !a_bMoreData ) {
		// No more data so remove padding if we are decrypting
		if ( !a_bEncrypt ) {
			int      iPadding;

			if ( a_ulDataLen == 0 ) {
				// Remove padding from previous block is current
				// block is zero length
				if ( g_pbOutData ) {
					iPadding = g_pbOutData[g_ulOutDataLen - 1];
					g_ulOutDataLen -= iPadding;
				}
			}
			else {
				// Remove padding from current block
				iPadding = a_pbData[a_ulDataLen - 1];
				a_ulDataLen -= iPadding;
			}
		}
	}

	// Write the previous buffer if there is one
	if ( g_pbOutData && ( g_ulOutDataLen > 0 ) )
		tWriteCount = fwrite( g_pbOutData, 1, g_ulOutDataLen, g_pOutFile );

	if ( a_bMoreData ) {
		// Allocate a (new) buffer if necessary
		if ( a_ulDataLen > g_ulOutBuffLen ) {
			free( g_pbOutData );

			g_ulOutBuffLen = a_ulDataLen;
			g_pbOutData = malloc( g_ulOutBuffLen );
			if ( !g_pbOutData ) {
				logError( TOKEN_MEMORY_ERROR );
				return -1;
			}
		}

		// Copy the current data to the holding buffer
		if ( a_ulDataLen > 0 )
			memcpy( g_pbOutData, a_pbData, a_ulDataLen );
		g_ulOutDataLen = a_ulDataLen;
	}
	else {
		// No more data so write the last piece of data
		if ( a_ulDataLen > 0 )
			tWriteCount = fwrite( a_pbData, 1, a_ulDataLen, g_pOutFile );

		fclose( g_pOutFile );
	}

	return 0;
}

int
main( int    a_iArgc,
      char **a_szArgv ) {

	int  rc = 1;

	char *pszPin = NULL;

	CK_RV              rv;
	CK_SESSION_HANDLE  hSession;
	CK_OBJECT_HANDLE   hObject;
	CK_MECHANISM       tMechanism = { CKM_AES_ECB, NULL, 0 };

	// Set up i18n
	initIntlSys( );

	// Parse the command
	if ( parseCmd( a_iArgc, a_szArgv ) == -1 )
		goto out;

	// Open the PKCS#11 TPM Token
	rv = openToken( g_pszToken );
	if ( rv != CKR_OK )
		goto out;

	// Make sure the token is initialized
	if ( !isTokenInitialized( ) ) {
		logMsg( TOKEN_NOT_INIT_ERROR );
		goto out;
	}

	// Open a session
	rv = openTokenSession( CKF_RW_SESSION, &hSession );
	if ( rv != CKR_OK )
		goto out;

	pszPin = getPlainPasswd( TOKEN_USER_PIN_PROMPT, FALSE );
	if ( !pszPin )
		goto out;

	// Login to the token
	rv = loginToken( hSession, CKU_USER, pszPin );
	if ( rv != CKR_OK )
		goto out;

	// Obtain the key
	if ( getKey( hSession, &hObject ) == -1 )
		goto out;

	// Perform the operation
	if ( g_bEncrypt )
		rv = encryptData( hSession, hObject, &tMechanism,
			readData, writeData );
	else
		rv = decryptData( hSession, hObject, &tMechanism,
			readData, writeData );

	if ( rv != CKR_OK )
		goto out;

	rc = 0;

out:
	shredPasswd( pszPin );

	if ( hSession )
		closeTokenSession( hSession );

	closeToken( );

	free( g_pszInFile );
	free( g_pszOutFile );
	free( g_pbInData );
	free( g_pbOutData );

	if ( rc == 0 )
		logInfo( TOKEN_CMD_SUCCESS, a_szArgv[ 0 ] );
	else
		logInfo( TOKEN_CMD_FAILED, a_szArgv[ 0 ] );

	return rc;
}
