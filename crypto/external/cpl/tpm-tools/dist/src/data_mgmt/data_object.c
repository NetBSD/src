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

#include "data_object.h"
#include "data_common.h"

#include <tpm_pkcs11.h>
#include <tpm_utils.h>

#include <stdlib.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>


/*
 * Global variables
 */
int   g_bPublic   = FALSE;		// Public object specifier
int   g_bExtended = FALSE;		// Extended information display specifier
char *g_pszToken  = NULL;		// Token label to be used

/*
 * parseCallback
 *   Process the command specific options.
 */
int
parseCallback( int         a_iOpt,
               const char *a_pszOptArg ) {

	switch ( a_iOpt ) {
		// Use the specified token label when finding the token
		case 'k':
			if ( !a_pszOptArg )
				return -1;

			g_pszToken = strdup( a_pszOptArg );
			break;

		case 'p':
			g_bPublic = TRUE;
			break;

		case 'x':
			g_bExtended = TRUE;
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

	logCmdHelp( a_pszCmd );
	logCmdOption( "-k, --token STRING",
			_("Use STRING to identify the label of the PKCS#11 token to be used") );
	logCmdOption( "-p, --public",
			_("Display only public objects") );
	logCmdOption( "-x, --extended",
			_("Display additional information about the objects") );
}

/*
 * parseCmd
 *   Parse the command line options.
 */
int
parseCmd( int    a_iArgc,
          char **a_pszArgv ) {

	char          *pszShortOpts = "k:px";
	struct option  stLongOpts[] = {
					{ "token", required_argument, NULL, 'k' },
					{ "public", no_argument, NULL, 'p' },
					{ "extended", no_argument, NULL, 'x' },
				};

	int  iNumLongOpts = sizeof( stLongOpts ) / sizeof( struct option );

	return genericOptHandler( a_iArgc, a_pszArgv,
					pszShortOpts, stLongOpts, iNumLongOpts,
					parseCallback, usageCallback );
}

int
main( int    a_iArgc,
      char **a_pszArgv ) {

	int  rc = 1;

	char *pszPin    = NULL;

	CK_RV              rv        = CKR_OK;
	CK_SESSION_HANDLE  hSession  = 0;
	CK_OBJECT_HANDLE  *hObject;
	CK_ULONG           ulCount;

	// Set up i18n
	initIntlSys( );

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

	// Open a session
	rv = openTokenSession( CKF_RW_SESSION, &hSession );
	if ( rv != CKR_OK )
		goto out;

	// Check the scope of the request, which will determine the login
	// requirements:
	//   Public  = no password, no login
	//   Private = user password, user login
	if ( !g_bPublic ) {
		pszPin = getPlainPasswd( TOKEN_USER_PIN_PROMPT, FALSE );
		if ( !pszPin )
			goto out;

		// Login to the token
		rv = loginToken( hSession, CKU_USER, pszPin );
		if ( rv != CKR_OK )
			goto out;
	}

	rv = findObjects( hSession, NULL, 0, &hObject, &ulCount );
	if ( rv != CKR_OK )
		goto out;

	if ( ulCount > 0 ) {
		while ( ulCount > 0 )
			displayObject( hSession, hObject[ --ulCount ], g_bExtended );
	}
	else {
		logMsg( NO_TOKEN_OBJECTS );
	}

	free( hObject );

	rc = 0;

out:
	shredPasswd( pszPin );

	if ( hSession )
		closeTokenSession( hSession );

	closeToken( );

	if ( rc == 0 )
		logInfo( TOKEN_CMD_SUCCESS, a_pszArgv[ 0 ] );
	else
		logInfo( TOKEN_CMD_FAILED, a_pszArgv[ 0 ] );

	return rc;
}
