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

#include "data_import.h"
#include "data_common.h"

#include <tpm_pkcs11.h>
#include <tpm_utils.h>

#include <stdlib.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <errno.h>

#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>
#include <openssl/evp.h>
#include <openssl/err.h>


/*
 * Global variables
 */
char *g_pszFile   = NULL;		// Object import file name
char *g_pszIdFile = NULL;		// Object identification file name
char *g_pszType   = NULL;		// Object import type
int   g_bPublic   = FALSE;		// Public object specifier
int   g_bYes      = FALSE;		// Yes/No prompt reply
char *g_pszToken  = NULL;		// Token label to be used

int       g_bAttrsValid  = FALSE;
CK_BYTE  *g_pchSubject   = NULL;	// SUBJECT attribute value
CK_LONG   g_subjectLen = 0;
CK_BYTE  *g_pchId        = NULL;	// ID attribute value
CK_ULONG  g_ulIdLen      = 0;
CK_BYTE  *g_pchName      = NULL;	// LABEL attribute value
CK_ULONG  g_ulNameLen    = 0;

/*
 * parseCallback
 *   Process the command specific options.
 */
int
parseCallback( int         a_iOpt,
               const char *a_pszOptArg ) {

	switch ( a_iOpt ) {
		// File with object to be used to obtain subject/id
		case 'i':
			if ( !a_pszOptArg )
				return -1;

			g_pszIdFile = strdup( a_pszOptArg );
			break;

		// Use the specified token label when finding the token
		case 'k':
			if ( !a_pszOptArg )
				return -1;

			g_pszToken = strdup( a_pszOptArg );
			break;

		// Name to use as the LABEL attribute value
		case 'n':
			if ( !a_pszOptArg )
				return -1;

			g_pchName = (CK_BYTE *)strdup( a_pszOptArg );
			g_ulNameLen = strlen( a_pszOptArg );
			break;

		// Make the object public
		case 'p':
			g_bPublic = TRUE;
			break;

		// Only import the specified object type
		case 't':
			if ( !a_pszOptArg )
				return -1;

			if ( ( strcmp( a_pszOptArg, TOKEN_OBJECT_KEY )  != 0 ) &&
			     ( strcmp( a_pszOptArg, TOKEN_OBJECT_CERT ) != 0 ) )
				return -1;

			g_pszType = strdup( a_pszOptArg );
			break;

		// Reply "yes" to any yes/no prompts
		case 'y':
			g_bYes = TRUE;
			break;
	}

	return 0;
}

/*
 * usageCallback
 *   Display command usage information.
 */
void
usageCallback( const char *a_pszCmd ) {

	char *pszArgs[2];
	char *pszArgsDesc[2];

	pszArgs[ 0 ] = "FILE";
	pszArgsDesc[ 0 ] = _("Import the PEM formatted RSA key and/or X.509 certificate object contained in FILE");
	pszArgs[ 1 ] = NULL;
	pszArgsDesc[ 1 ] = NULL;

	logCmdHelpEx( a_pszCmd, pszArgs, pszArgsDesc );
	logCmdOption( "-i, --idfile FILE",
			_("Use FILE as the PEM formatted X.509 certificate input used to obtain the subject and id attributes") );
	logCmdOption( "-k, --token STRING",
			_("Use STRING to identify the label of the PKCS#11 token to be used") );
	logCmdOption( "-n, --name STRING",
			_("Use STRING as the label for the imported object(s)") );
	logCmdOption( "-p, --public",
			_("Import the object(s) as a public object") );
	logCmdOption( "-t, --type key|cert",
			_("Import only the specified object type") );
	logCmdOption( "-y, --yes",
			_("Assume yes as the answer to any confirmation prompt") );
}

/*
 * parseCmd
 *   Parse the command line options.
 */
int
parseCmd( int    a_iArgc,
          char **a_pszArgv ) {

	int  rc;

	char          *pszShortOpts  = "i:k:n:pt:y";
	struct option  stLongOpts[]  = {
			{ "idfile", required_argument, NULL, 'i' },
			{ "name", required_argument, NULL, 'n' },
			{ "public", no_argument, NULL, 'p' },
			{ "token", required_argument, NULL, 'k' },
			{ "type", required_argument, NULL, 't' },
			{ "yes", no_argument, NULL, 'y' },
		};
	int  iNumLongOpts = sizeof( stLongOpts ) / sizeof( struct option );

	rc = genericOptHandler( a_iArgc, a_pszArgv,
		pszShortOpts, stLongOpts, iNumLongOpts,
		parseCallback, usageCallback );
	if ( rc == -1 )
		return -1;

	if ( optind >= a_iArgc ) {
		logMsg( TOKEN_FILE_ERROR );
		usageCallback( a_pszArgv[ 0 ] );
		return -1;
	}

	g_pszFile = strdup( a_pszArgv[ optind ] );

	return 0;
}

/*
 * findExistingObjects
 *   Search for objects of the supplied type that have a SUBJECT
 *   and ID attribute equal to the values of the object to be
 *   imported.
 */
int
findExistingObjects( CK_SESSION_HANDLE   a_hSession,
                     CK_ATTRIBUTE       *a_tAttr,
                     CK_ULONG            a_ulAttrCount,
                     CK_OBJECT_HANDLE  **a_phObject,
                     CK_ULONG           *a_pulObjectCount ) {

	CK_RV     rv;
	CK_BBOOL  bTrue = TRUE;

	// Set up default search attributes
	CK_ATTRIBUTE  tDefaultAttr[]     = {
			{ CKA_TOKEN, &bTrue, sizeof( bTrue ) },
			{ CKA_SUBJECT, g_pchSubject, g_subjectLen },
			{ CKA_ID, g_pchId, g_ulIdLen },
		};
	CK_ULONG      ulDefaultAttrCount = sizeof( tDefaultAttr ) / sizeof( CK_ATTRIBUTE );

	CK_ATTRIBUTE  tAttr[ ulDefaultAttrCount + a_ulAttrCount ];
	CK_ULONG      ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );

	*a_phObject = NULL;
	*a_pulObjectCount = 0;

	// Apply attributes and search
	memcpy( tAttr, tDefaultAttr, ulDefaultAttrCount * sizeof( CK_ATTRIBUTE ) );
	if ( a_ulAttrCount )
		memcpy( tAttr + ulDefaultAttrCount, a_tAttr, a_ulAttrCount * sizeof( CK_ATTRIBUTE ) );

	rv = findObjects( a_hSession, tAttr, ulAttrCount, a_phObject, a_pulObjectCount );

	return ( rv == CKR_OK ) ? 0 : -1;
}

/*
 * checkExistingObjects
 *   Use findExistingObjects to determine if objects of the
 *   supplied type currently exist.  If so, prompt the user as
 *   to whether to replace the existing objects.
 */
int
checkExistingObjects( CK_SESSION_HANDLE  a_hSession,
                      CK_ATTRIBUTE      *a_tAttr,
                      CK_ULONG           a_ulAttrCount,
                      const char        *a_pszObject ) {

	int  rc = -1;

	CK_OBJECT_HANDLE *phObject      = NULL;
	CK_ULONG          ulObjectCount = 0;

	char  szPrompt[ strlen( TOKEN_ID_PROMPT ) + strlen( a_pszObject ) + 1 ];
	char *pszReply = NULL;

	if ( g_bAttrsValid ) {
		// Search for existing objects
		if ( findExistingObjects( a_hSession, a_tAttr, a_ulAttrCount, &phObject, &ulObjectCount ) == -1 )
			goto out;

		if ( ulObjectCount > 0 ) {
			// One or more objects exists
			if ( !g_bYes ) {
				// Prompt for whether to replace the existing objects
				sprintf( szPrompt, TOKEN_ID_PROMPT, a_pszObject );
				pszReply = getReply( szPrompt, 1 );
				if ( !pszReply ||
					( strlen( pszReply ) == 0 ) ||
					( strcasecmp( pszReply, TOKEN_ID_NO ) == 0 ) ) {
					goto out;
				}
			}
		}
	}

	rc = 0;

out:
	free( phObject );
	free( pszReply );

	return rc;
}

/*
 * destroyExistingObjects
 *   Use findExistingObjects to locate all objects of the
 *   supplied type that currently exists and destroy them.
 */
int
destroyExistingObjects( CK_SESSION_HANDLE  a_hSession,
                        CK_ATTRIBUTE      *a_tAttr,
                        CK_ULONG           a_ulAttrCount ) {

	int  rc = -1;

	CK_RV             rv;
	CK_OBJECT_HANDLE *phObject      = NULL;
	CK_ULONG          ulObjectCount = 0;

	if ( g_bAttrsValid ) {
		// Search for existing objects
		if ( findExistingObjects( a_hSession, a_tAttr, a_ulAttrCount, &phObject, &ulObjectCount ) == -1 )
			goto out;

		// Destroy each object found
		while ( ulObjectCount > 0 ) {
			rv = destroyObject( a_hSession, phObject[ --ulObjectCount ] );
			if ( rv != CKR_OK )
				goto out;
		}
	}

	rc = 0;

out:
	free( phObject );

	return rc;
}

/*
 * readX509Cert
 *   Use the OpenSSL library to read a PEM formatted X509 certificate.
 */
int
readX509Cert( const char  *a_pszFile,
              int          a_bCheckKey,
              X509       **a_pX509 ) {

	int rc = -1;

	FILE     *pFile = stdin;
	X509     *pX509 = NULL;
	EVP_PKEY *pKey  = NULL;

	*a_pX509 = NULL;

	// Open the file to be read
	if ( a_pszFile ) {
		errno = 0;
		pFile = fopen( a_pszFile, "r" );
		if ( !pFile ) {
			logError( TOKEN_FILE_OPEN_ERROR, a_pszFile, strerror( errno ) );
			goto out;
		}
	}

	// Read the X509 certificate
	pX509 = PEM_read_X509( pFile, NULL, NULL, NULL );
	if ( !pX509 ) {
		unsigned long  ulError = ERR_get_error( );

		// Not necessarily an error if the file doesn't contain the cert
		if ( ( ERR_GET_LIB( ulError ) == ERR_R_PEM_LIB ) &&
		     ( ERR_GET_REASON( ulError ) == PEM_R_NO_START_LINE ) ) {
			logInfo( TOKEN_OPENSSL_ERROR, ERR_error_string( ulError, NULL ) );
			rc = 0;
		}
		else
			logError( TOKEN_OPENSSL_ERROR, ERR_error_string( ulError, NULL ) );

		goto out;
	}

	// Make sure the certificate uses an RSA key
	if ( !a_bCheckKey ) {
		rc = 0;
		goto out;
	}

	pKey = X509_get_pubkey( pX509 );
	if ( !pKey ) {
		logInfo( TOKEN_OPENSSL_ERROR,
			ERR_error_string( ERR_get_error( ), NULL ) );

		X509_free( pX509 );
		pX509 = NULL;
		goto out;
	}

	if ( EVP_PKEY_type( pKey->type ) != EVP_PKEY_RSA ) {
		logError( TOKEN_RSA_KEY_ERROR );

		X509_free( pX509 );
		pX509 = NULL;
		goto out;
	}

	rc = 0;

out:
	*a_pX509 = pX509;

	if ( a_pszFile && pFile )
		fclose( pFile );

	return rc;
}

/*
 * checkX509Cert
 *   Use checkExistingObjects to search for X_509 objects
 *   that match the attributes of the X_509 object to be imported.
 */
int
checkX509Cert( CK_SESSION_HANDLE  a_hSession ) {

	CK_CERTIFICATE_TYPE  tX509       = CKC_X_509;
	CK_ATTRIBUTE         tAttr[]     = {
			{ CKA_CERTIFICATE_TYPE, &tX509, sizeof( tX509 ) },
		};
	CK_ULONG             ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );

	return checkExistingObjects( a_hSession, tAttr, ulAttrCount, TOKEN_ID_X509_CERT );
}

/*
 * destroyX509CertObject
 *   Use destroyExistingObjects to destroy X_509 objects
 *   that match the attributes of the X_509 object to be imported.
 */
int
destroyX509CertObject( CK_SESSION_HANDLE  a_hSession ) {

	CK_CERTIFICATE_TYPE  tX509       = CKC_X_509;
	CK_ATTRIBUTE         tAttr[]     = {
			{ CKA_CERTIFICATE_TYPE, &tX509, sizeof( tX509 ) },
		};
	CK_ULONG             ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );

	return destroyExistingObjects( a_hSession, tAttr, ulAttrCount );
}

/*
 * createX509CertObject
 *   Create an X_509 object.
 */
int
createX509CertObject( X509              *a_pX509,
                      CK_SESSION_HANDLE  a_hSession ) {

	int  rc = -1;

	CK_RV  rv;

	CK_BBOOL  bTrue = TRUE;

	X509_NAME    *pIssuer    = NULL;
	ASN1_INTEGER *pSerialNum = NULL;

	CK_BYTE  *pchIssuer      = NULL;
	CK_LONG  issuerLen    = 0;
	CK_BYTE  *pchSerialNum   = NULL;
	CK_LONG  serialNumLen = 0;
	CK_BYTE  *pchCert        = NULL;
	CK_LONG  certLen      = 0;

	CK_OBJECT_CLASS      clCertClass = CKO_CERTIFICATE;
	CK_CERTIFICATE_TYPE  tCertType   = CKC_X_509;
	CK_BBOOL             bPrivate    = ( !g_bPublic ) ? TRUE : FALSE;

	// The issuer, serial number, and value attributes must be completed
	// before the object is created
	CK_ATTRIBUTE  tCertAttr[] = {
			{ CKA_CLASS, &clCertClass, sizeof( clCertClass ) },
			{ CKA_TOKEN, &bTrue, sizeof( bTrue ) },
			{ CKA_PRIVATE, &bPrivate, sizeof( bPrivate ) },
			{ CKA_MODIFIABLE, &bTrue, sizeof( bTrue ) },
			{ CKA_LABEL, g_pchName, g_ulNameLen },
			{ CKA_CERTIFICATE_TYPE, &tCertType, sizeof( tCertType ) },
			{ CKA_SUBJECT, g_pchSubject, g_subjectLen },
			{ CKA_ID, g_pchId, g_ulIdLen },
			{ CKA_ISSUER, NULL, 0 },
			{ CKA_SERIAL_NUMBER, NULL, 0 },
			{ CKA_VALUE, NULL, 0 },
		};
	CK_ULONG  ulCertAttrCount = sizeof( tCertAttr ) / sizeof( CK_ATTRIBUTE );

	CK_OBJECT_HANDLE  hObject;

	// Get the issuer name from the X509 certificate
	pIssuer = X509_get_issuer_name( a_pX509 );
	if ( !pIssuer ) {
		logError( TOKEN_OPENSSL_ERROR,
				ERR_error_string( ERR_get_error( ), NULL ) );
		goto out;
	}
	issuerLen = i2d_X509_NAME( pIssuer, &pchIssuer );
	if ( issuerLen < 0 ) {
		logError( TOKEN_OPENSSL_ERROR,
				ERR_error_string( ERR_get_error( ), NULL ) );
		goto out;
	}

	// Get the serial number from the X509 certificate
	pSerialNum = X509_get_serialNumber( a_pX509 );
	if ( !pSerialNum ) {
		logError( TOKEN_OPENSSL_ERROR,
				ERR_error_string( ERR_get_error( ), NULL ) );
		goto out;
	}
	serialNumLen = i2d_ASN1_INTEGER( pSerialNum, &pchSerialNum );
	if ( serialNumLen < 0 ) {
		logError( TOKEN_OPENSSL_ERROR,
				ERR_error_string( ERR_get_error( ), NULL ) );
		goto out;
	}

	// Get a DER encoded format of the X509 certificate
	certLen = i2d_X509( a_pX509, &pchCert );
	if ( certLen < 0 ) {
		logError( TOKEN_OPENSSL_ERROR,
				ERR_error_string( ERR_get_error( ), NULL ) );
		goto out;
	}

	// Set the attribute values
	tCertAttr[ 8 ].pValue = pchIssuer;
	tCertAttr[ 8 ].ulValueLen = issuerLen;
	tCertAttr[ 9 ].pValue = pchSerialNum;
	tCertAttr[ 9 ].ulValueLen = serialNumLen;
	tCertAttr[ 10 ].pValue = pchCert;
	tCertAttr[ 10 ].ulValueLen = certLen;

	// Create the X509 certificate object
	rv = createObject( a_hSession, tCertAttr, ulCertAttrCount, &hObject );
	if ( rv != CKR_OK )
		goto out;

	rc = 0;

out:
	OPENSSL_free( pchIssuer );
	OPENSSL_free( pchCert );
	OPENSSL_free( pchSerialNum );

	return rc;
}

/*
 * doX509Cert
 *   Process an X509 certificate for import.
 */
int
doX509Cert( X509              *a_pX509,
            CK_SESSION_HANDLE  a_hSession ) {

	int  rc = -1;

	if ( destroyX509CertObject( a_hSession ) == -1 )
		goto out;

	if ( createX509CertObject( a_pX509, a_hSession ) == -1 )
		goto out;

	rc = 0;

out:
	return rc;
}

/*
 * readRsaKey
 *   Use the OpenSSL library to read a PEM formatted RSA key.
 */
int
readRsaKey( const char  *a_pszFile,
            RSA        **a_pRsa ) {

	int  rc = -1;

	FILE *pFile = stdin;
	RSA  *pRsa  = NULL;

	*a_pRsa = NULL;

	// Open the file to be read
	if ( a_pszFile ) {
		errno = 0;
		pFile = fopen( a_pszFile, "r" );
		if ( !pFile ) {
			logError( TOKEN_FILE_OPEN_ERROR, a_pszFile, strerror( errno ) );
			goto out;
		}
	}

	// Read the RSA key
	//   This reads the public key also, not just the private key
	pRsa = PEM_read_RSAPrivateKey( pFile, NULL, NULL, NULL );
	if ( !pRsa ) {
		unsigned long  ulError = ERR_get_error( );

		// Not necessarily an error if the file doesn't contain the key
		if ( ( ERR_GET_LIB( ulError ) == ERR_R_PEM_LIB ) &&
		     ( ERR_GET_REASON( ulError ) == PEM_R_NO_START_LINE ) ) {
			logInfo( TOKEN_OPENSSL_ERROR, ERR_error_string( ulError, NULL ) );
			rc = 0;
		}
		else
			logError( TOKEN_OPENSSL_ERROR, ERR_error_string( ulError, NULL ) );

		goto out;
	}

	rc = 0;

out:
	if ( a_pszFile && pFile )
		fclose( pFile );

	*a_pRsa = pRsa;

	return rc;
}

/*
 * checkRsaPubKey
 *   Use checkExistingObjects to search for RSA public key objects
 *   that match the attributes of the X509's RSA public key object
 *   to be imported.
 */
int
checkRsaPubKey( CK_SESSION_HANDLE  a_hSession ) {

	CK_OBJECT_CLASS  tPubKey  = CKO_PUBLIC_KEY;
	CK_KEY_TYPE      tRsa     = CKK_RSA;
	CK_ATTRIBUTE     tAttr[]  = {
			{ CKA_CLASS, &tPubKey, sizeof( tPubKey ) },
			{ CKA_KEY_TYPE, &tRsa, sizeof( tRsa ) },
		};
	CK_ULONG         ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );

	return checkExistingObjects( a_hSession, tAttr, ulAttrCount, TOKEN_ID_RSA_PUBKEY );
}

/*
 * checkRsaKey
 *   Use checkExistingObjects to search for RSA objects
 *   that match the attributes of the RSA object to be imported.
 */
int
checkRsaKey( CK_SESSION_HANDLE  a_hSession ) {

	CK_KEY_TYPE   tRsa        = CKK_RSA;
	CK_ATTRIBUTE  tAttr[]     = {
			{ CKA_KEY_TYPE, &tRsa, sizeof( tRsa ) },
		};
	CK_ULONG      ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );

	return checkExistingObjects( a_hSession, tAttr, ulAttrCount, TOKEN_ID_RSA_KEY );
}

/*
 * destroyRsaKeyObject
 *   Use destroyExistingObjects to destroy RSA objects
 *   that match the attributes of the RSA object to be imported.
 */
int
destroyRsaPubKeyObject( CK_SESSION_HANDLE  a_hSession ) {

	CK_OBJECT_CLASS  tPubKey     = CKO_PUBLIC_KEY;
	CK_KEY_TYPE      tRsa        = CKK_RSA;
	CK_ATTRIBUTE     tAttr[]     = {
			{ CKA_CLASS, &tPubKey, sizeof( tPubKey ) },
			{ CKA_KEY_TYPE, &tRsa, sizeof( tRsa ) },
		};
	CK_ULONG         ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );

	return destroyExistingObjects( a_hSession, tAttr, ulAttrCount );
}

/*
 * destroyRsaKeyObject
 *   Use destroyExistingObjects to destroy RSA objects
 *   that match the attributes of the RSA object to be imported.
 */
int
destroyRsaKeyObject( CK_SESSION_HANDLE  a_hSession ) {

	CK_KEY_TYPE   tRsa        = CKK_RSA;
	CK_ATTRIBUTE  tAttr[]     = {
			{ CKA_KEY_TYPE, &tRsa, sizeof( tRsa ) },
		};
	CK_ULONG      ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );

	return destroyExistingObjects( a_hSession, tAttr, ulAttrCount );
}

/*
 * createRsaPubKeyObject
 *   Create an RSA public key object.
 */
int
createRsaPubKeyObject( RSA               *a_pRsa,
                       CK_SESSION_HANDLE  a_hSession,
                       CK_OBJECT_HANDLE  *a_hObject ) {

	int  rc = -1;

	int  nLen = BN_num_bytes( a_pRsa->n );
	int  eLen = BN_num_bytes( a_pRsa->e );

	CK_RV  rv;

	CK_BBOOL  bTrue  = TRUE;
	CK_BBOOL  bFalse = FALSE;

	CK_BYTE *n = malloc( nLen );
	CK_BYTE *e = malloc( eLen );

	CK_OBJECT_CLASS  clPubClass  = CKO_PUBLIC_KEY;
	CK_KEY_TYPE      tKeyType    = CKK_RSA;
	CK_BBOOL         bPrivate    = ( !g_bPublic ) ? TRUE : FALSE;

	CK_ATTRIBUTE  tAttr[] = {
			{ CKA_CLASS, &clPubClass, sizeof( clPubClass ) },
			{ CKA_TOKEN, &bTrue, sizeof( bTrue ) },
			{ CKA_PRIVATE, &bPrivate, sizeof( bPrivate ) },
			{ CKA_MODIFIABLE, &bTrue, sizeof( bTrue ) },
			{ CKA_LABEL, g_pchName, g_ulNameLen },
			{ CKA_KEY_TYPE, &tKeyType, sizeof( tKeyType ) },
			{ CKA_ID, g_pchId, g_ulIdLen },
			{ CKA_SUBJECT, g_pchSubject, g_subjectLen },
			{ CKA_ENCRYPT, &bTrue, sizeof( bTrue ) },
			{ CKA_VERIFY, &bTrue, sizeof( bTrue ) },
			{ CKA_VERIFY_RECOVER, &bFalse, sizeof( bFalse ) },
			{ CKA_WRAP, &bFalse, sizeof( bFalse ) },
			{ CKA_MODULUS, n, nLen },
			{ CKA_PUBLIC_EXPONENT, e, eLen },
		};
	CK_ULONG  ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );

	*a_hObject = 0;

	if ( !n || !e ) {
		logError( TOKEN_MEMORY_ERROR );
		goto out;
	}

	// Get binary representations of the RSA key information
	BN_bn2bin( a_pRsa->n, n );
	BN_bn2bin( a_pRsa->e, e );

	// Create the RSA public key object
	rv = createObject( a_hSession, tAttr, ulAttrCount, a_hObject );
	if ( rv != CKR_OK )
		goto out;

	rc = 0;

out:
	free( n );
	free( e );

	return rc;
}

/*
 * createRsaPrivKeyObject
 *   Create an RSA private key object.
 */
int
createRsaPrivKeyObject( RSA               *a_pRsa,
                        CK_SESSION_HANDLE  a_hSession,
                        CK_OBJECT_HANDLE  *a_hObject ) {

	int  rc = -1;

	int  nLen = BN_num_bytes( a_pRsa->n );
	int  eLen = BN_num_bytes( a_pRsa->e );
	int  dLen = BN_num_bytes( a_pRsa->d );
	int  pLen = BN_num_bytes( a_pRsa->p );
	int  qLen = BN_num_bytes( a_pRsa->q );
	int  dmp1Len = BN_num_bytes( a_pRsa->dmp1 );
	int  dmq1Len = BN_num_bytes( a_pRsa->dmq1 );
	int  iqmpLen = BN_num_bytes( a_pRsa->iqmp );

	CK_RV  rv;

	CK_BBOOL  bTrue  = TRUE;
	CK_BBOOL  bFalse = FALSE;

	CK_BYTE *n = malloc( nLen );
	CK_BYTE *e = malloc( eLen );
	CK_BYTE *d = malloc( dLen );
	CK_BYTE *p = malloc( pLen );
	CK_BYTE *q = malloc( qLen );
	CK_BYTE *dmp1 = malloc( dmp1Len );
	CK_BYTE *dmq1 = malloc( dmq1Len );
	CK_BYTE *iqmp = malloc( iqmpLen );

	CK_OBJECT_CLASS  clPrivClass = CKO_PRIVATE_KEY;
	CK_KEY_TYPE      tKeyType    = CKK_RSA;
	CK_BBOOL         bPrivate    = ( !g_bPublic ) ? TRUE : FALSE;

	CK_ATTRIBUTE  tAttr[] = {
			{ CKA_CLASS, &clPrivClass, sizeof( clPrivClass ) },
			{ CKA_TOKEN, &bTrue, sizeof( bTrue ) },
			{ CKA_PRIVATE, &bPrivate, sizeof( bPrivate ) },
			{ CKA_MODIFIABLE, &bTrue, sizeof( bTrue ) },
			{ CKA_LABEL, g_pchName, g_ulNameLen },
			{ CKA_KEY_TYPE, &tKeyType, sizeof( tKeyType ) },
			{ CKA_ID, g_pchId, g_ulIdLen },
			{ CKA_SUBJECT, g_pchSubject, g_subjectLen },
			{ CKA_SENSITIVE, &bTrue, sizeof( bTrue ) },
			{ CKA_DECRYPT, &bTrue, sizeof( bTrue ) },
			{ CKA_SIGN, &bTrue, sizeof( bTrue ) },
			{ CKA_SIGN_RECOVER, &bFalse, sizeof( bFalse ) },
			{ CKA_UNWRAP, &bFalse, sizeof( bFalse ) },
			{ CKA_EXTRACTABLE, &bFalse, sizeof( bFalse ) },
			{ CKA_MODULUS, n, nLen },
			{ CKA_PUBLIC_EXPONENT, e, eLen },
			{ CKA_PRIVATE_EXPONENT, d, dLen },
			{ CKA_PRIME_1, p, pLen },
			{ CKA_PRIME_2, q, qLen },
			{ CKA_EXPONENT_1, dmp1, dmp1Len },
			{ CKA_EXPONENT_2, dmq1, dmq1Len },
			{ CKA_COEFFICIENT, iqmp, iqmpLen },
		};
	CK_ULONG  ulAttrCount = sizeof( tAttr ) / sizeof( CK_ATTRIBUTE );

	*a_hObject = 0;

	if ( !n || !e || !d || !p || !q || !dmp1 || !dmq1 || !iqmp ) {
		logError( TOKEN_MEMORY_ERROR );
		goto out;
	}

	// Get binary representations of the RSA key information
	BN_bn2bin( a_pRsa->n, n );
	BN_bn2bin( a_pRsa->e, e );
	BN_bn2bin( a_pRsa->d, d );
	BN_bn2bin( a_pRsa->p, p );
	BN_bn2bin( a_pRsa->q, q );
	BN_bn2bin( a_pRsa->dmp1, dmp1 );
	BN_bn2bin( a_pRsa->dmq1, dmq1 );
	BN_bn2bin( a_pRsa->iqmp, iqmp );

	// Create the RSA private key object
	rv = createObject( a_hSession, tAttr, ulAttrCount, a_hObject );
	if ( rv != CKR_OK )
		goto out;

	rc = 0;

out:
	free( n );
	free( e );
	free( d );
	free( p );
	free( q );
	free( dmp1 );
	free( dmq1 );
	free( iqmp );

	return rc;
}

/*
 * createRsaKeyObject
 *   Create an RSA key object (both public and private).
 */
int
createRsaKeyObject( RSA               *a_pRsa,
                    CK_SESSION_HANDLE  a_hSession ) {

	int  rc = -1;

	CK_OBJECT_HANDLE  hPubObject;
	CK_OBJECT_HANDLE  hPrivObject;

	// Create the RSA public key object
	if ( createRsaPubKeyObject( a_pRsa, a_hSession, &hPubObject ) == -1 )
		goto out;

	// Create the RSA private key object
	if ( createRsaPrivKeyObject( a_pRsa, a_hSession, &hPrivObject ) == -1 ) {
		// Private key object creation failed, destroy the public
		// key object just created
		destroyObject( a_hSession, hPubObject );
		goto out;
	}

	rc = 0;

out:
	return rc;
}

/*
 * doRsaPubKey
 *   Process an RSA public key for import.
 */
int
doRsaPubKey( RSA               *a_pRsa,
             CK_SESSION_HANDLE  a_hSession ) {

	int  rc = -1;

	CK_OBJECT_HANDLE  hObject;

	if ( destroyRsaPubKeyObject( a_hSession ) == -1 )
		goto out;

	if ( createRsaPubKeyObject( a_pRsa, a_hSession, &hObject ) == -1 )
		goto out;

	rc = 0;

out:
	return rc;
}

/*
 * doRsaKey
 *   Process an RSA key for import.
 */
int
doRsaKey( RSA               *a_pRsa,
          CK_SESSION_HANDLE  a_hSession ) {

	int  rc = -1;

	if ( destroyRsaKeyObject( a_hSession ) == -1 )
		goto out;

	if ( createRsaKeyObject( a_pRsa, a_hSession ) == -1 )
		goto out;

	rc = 0;

out:
	return rc;
}

/*
 * getSubjectId
 *   Extract the subject name and key identifier from an
 *   X509 certificate for use as the SUBJECT and ID attributes.
 */
int
getSubjectId( X509 *a_pX509 ) {

	int  rc = -1;

	char *pszReply = NULL;

	X509              *pX509    = a_pX509;
	X509_NAME         *pSubject = NULL;
	ASN1_OCTET_STRING *pSkid    = NULL;

	// Use the Id input file if specified
	if ( g_pszIdFile )
		if ( readX509Cert( g_pszIdFile, FALSE, &pX509 ) == -1 )
			goto out;

	if ( !pX509 ) {
		// Prompt the user about creating without it.
		if ( !g_bYes ) {
			// Prompt for whether to import without the attributes
			pszReply = getReply( TOKEN_ID_MISSING_PROMPT, 1 );
			if ( !pszReply ||
				( strlen( pszReply ) == 0 ) ||
				( strcasecmp( pszReply, TOKEN_ID_NO ) == 0 ) ) {
				goto out;
			}
		}

		rc = 0;
		goto out;
	}

	// Get the subject name from the X509 certificate
	pSubject = X509_get_subject_name( pX509 );
	if ( !pSubject ) {
		logInfo( TOKEN_OPENSSL_ERROR,
				ERR_error_string( ERR_get_error( ), NULL ) );
		goto out;
	}

	// Get the DER encoded format of the subject name
	g_subjectLen = i2d_X509_NAME( pSubject, &g_pchSubject );
	if ( g_subjectLen < 0 ) {
		logInfo( TOKEN_OPENSSL_ERROR,
				ERR_error_string( ERR_get_error( ), NULL ) );
		goto out;
	}

	// Get the subject key identifier from the X509 certficate
	pSkid = X509_get_ext_d2i( pX509, NID_subject_key_identifier, NULL, NULL );
	if ( !pSkid ) {
		logInfo( TOKEN_OPENSSL_ERROR,
				ERR_error_string( ERR_get_error( ), NULL ) );
		goto out;
	}

	// Get the ASCII string format of the subject key identifier
	g_pchId = (CK_BYTE *)i2s_ASN1_OCTET_STRING( NULL, pSkid );
	if ( !g_pchId ) {
		logInfo( TOKEN_OPENSSL_ERROR,
				ERR_error_string( ERR_get_error( ), NULL ) );
		goto out;
	}
	g_ulIdLen = strlen( (char *)g_pchId );

	g_bAttrsValid = TRUE;

	rc = 0;

out:
	// Free the structure if it was created for this function
	if ( pX509 && ( pX509 != a_pX509 ) )
		X509_free( pX509 );

	ASN1_OCTET_STRING_free( pSkid );
	free( pszReply );

	return rc;
}

int
main( int    a_iArgc,
      char **a_pszArgv ) {

	int  rc = 1;

	char *pszPin    = NULL;

	CK_RV              rv        = CKR_OK;
	CK_SESSION_HANDLE  hSession  = 0;

	X509 *pX509   = NULL;
	RSA  *pPubRsa = NULL;
	RSA  *pRsa    = NULL;

	// Set up i18n
	initIntlSys( );

	// Initialize OpenSSL
	OpenSSL_add_all_algorithms( );
	ERR_load_crypto_strings( );

	// Parse the command
	if ( parseCmd( a_iArgc, a_pszArgv ) == -1 )
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

	// Create the structures based on the input
	if ( !g_pszType ) {
		if ( readX509Cert( g_pszFile, TRUE, &pX509 ) == -1 )
			goto out;
		if ( readRsaKey( g_pszFile, &pRsa ) == -1 )
			goto out;
		if ( !pX509 && !pRsa ) {
			logError( TOKEN_OBJECT_ERROR );
			goto out;
		}
	}
	else if ( strcmp( g_pszType, TOKEN_OBJECT_CERT ) == 0 ) {
		if ( readX509Cert( g_pszFile, TRUE, &pX509 ) == -1 )
			goto out;
		if ( !pX509 ) {
			logError( TOKEN_OBJECT_ERROR );
			goto out;
		}
	}
	else if ( strcmp( g_pszType, TOKEN_OBJECT_KEY ) == 0 ) {
		if ( readRsaKey( g_pszFile, &pRsa ) == -1 )
			goto out;
		if ( !pRsa ) {
			logError( TOKEN_OBJECT_ERROR );
			goto out;
		}
	}

	// Open a session
	rv = openTokenSession( CKF_RW_SESSION, &hSession );
	if ( rv != CKR_OK )
		goto out;

	// Check the scope of the request, which will determine the login
	// requirements:
	//   Public  = no password, no login
	//   Private = user password, user login (default)
	if ( !g_bPublic ) {
		pszPin = getPlainPasswd( TOKEN_USER_PIN_PROMPT, FALSE );
		if ( !pszPin )
			goto out;

		// Login to the token
		rv = loginToken( hSession, CKU_USER, pszPin );
		if ( rv != CKR_OK )
			goto out;
	}

	// Obtain the subject name and id, these are used to
	// uniquely identify the certificate/key relation
	if ( getSubjectId( pX509 ) == -1 ) {
		logError( TOKEN_ID_ERROR );
		goto out;
	}

	// Now check for existing objects that may get replaced
	// prior to processing the request(s)
	if ( pX509 ) {
		if ( checkX509Cert( hSession ) == -1 ) {
			goto out;
		}

		// If we are not importing any RSA keys, use the
		// public key from the certificate
		if ( !pRsa ) {
			if ( checkRsaPubKey( hSession ) == -1 ) {
				goto out;
			}
		}

		pPubRsa = EVP_PKEY_get1_RSA( X509_get_pubkey( pX509 ) );
	}
	if ( pRsa ) {
		if ( checkRsaKey( hSession ) == -1 ) {
			goto out;
		}
	}

	// Process the request(s)
	if ( pX509 ) {
		if ( doX509Cert( pX509, hSession ) == -1 )
			goto out;

		// If we are not importing any RSA keys, use the
		// public key from the certificate
		if ( !pRsa ) {
			if ( doRsaPubKey( pPubRsa, hSession ) == -1 )
				goto out;
		}
	}
	if ( pRsa ) {
		if ( doRsaKey( pRsa, hSession ) == -1 )
			goto out;
	}

	rc = 0;

out:
	shredPasswd( pszPin );

	if ( hSession )
		closeTokenSession( hSession );

	closeToken( );

	free( g_pszFile );
	free( g_pszIdFile );
	free( g_pszType );
	X509_free( pX509 );
	RSA_free( pRsa );
	OPENSSL_free( g_pchSubject );
	OPENSSL_free( g_pchId );
	free( g_pchName );

	EVP_cleanup( );

	if ( rc == 0 )
		logInfo( TOKEN_CMD_SUCCESS, a_pszArgv[ 0 ] );
	else
		logInfo( TOKEN_CMD_FAILED, a_pszArgv[ 0 ] );

	return rc;
}
