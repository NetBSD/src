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

#include "data_passwd.h"
#include "data_common.h"

#include <tpm_pkcs11.h>
#include <tpm_utils.h>

#include <stdlib.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>


/*
 * Global variables
 */
BOOL  g_bSystem   = FALSE;		// Set SO pin specifier
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

		case 's':
			g_bSystem = TRUE;
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
	logCmdOption( "-s, --security-officer",
			_("Change the security officer password") );
}

/*
 * parseCmd
 *   Parse the command line options.
 */
int
parseCmd( int    a_iArgc,
          char **a_pszArgv ) {

	char *pszShortOpts = "k:s";
	struct option  stLongOpts[] = {
					{ "token", required_argument, NULL, 'k' },
					{ "security-officer", no_argument, NULL, 's' },
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

	// Create buffers for PIN prompts for formatting using sprintf
	char  szSoNewPinPrompt[ strlen( TOKEN_SO_NEW_PIN_PROMPT ) + 16 ];
	char  szUserNewPinPrompt[ strlen( TOKEN_USER_NEW_PIN_PROMPT ) + 16 ];

	char *pszPrompt = NULL;
	char *pszPin    = NULL;
	char *pszNewPin = NULL;

	CK_RV              rv        = CKR_OK;
	CK_USER_TYPE       tUser     = CKU_USER;
	CK_SESSION_HANDLE  hSession  = 0;

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

	// Get the current password
	if ( g_bSystem ) {
		pszPrompt = TOKEN_SO_PIN_PROMPT;
		tUser = CKU_SO;
	}
	else {
		pszPrompt = TOKEN_USER_PIN_PROMPT;
		tUser = CKU_USER;
	}
	pszPin = getPlainPasswd( pszPrompt, FALSE );
	if ( !pszPin )
		goto out;

	// Login to the token
	rv = loginToken( hSession, tUser, pszPin );
	if ( rv != CKR_OK )
		goto out;

	// Get the new password
	if ( g_bSystem ) {
		sprintf( szSoNewPinPrompt, TOKEN_SO_NEW_PIN_PROMPT, getMinPinLen( ), getMaxPinLen( ) );
		pszPrompt = szSoNewPinPrompt;
	}
	else {
		sprintf( szUserNewPinPrompt, TOKEN_USER_NEW_PIN_PROMPT, getMinPinLen( ), getMaxPinLen( ) );
		pszPrompt = szUserNewPinPrompt;
	}

	while ( TRUE ) {
		// Prompt for a new SO password
		pszNewPin = getPlainPasswd( pszPrompt, TRUE );
		if ( !pszNewPin )
			goto out;

		// Set the new password
		rv = setPin( hSession, pszPin, pszNewPin );
		if ( rv == CKR_OK )
			break;

		if ( ( rv == CKR_PIN_INVALID ) || ( rv == CKR_PIN_LEN_RANGE ) )
			logError( TOKEN_INVALID_PIN );
		else
			goto out;

		shredPasswd( pszNewPin );
	}

	rc = 0;

out:
	shredPasswd( pszPin );
	shredPasswd( pszNewPin );

	if ( hSession )
		closeTokenSession( hSession );

	closeToken( );

	if ( rc == 0 )
		logInfo( TOKEN_CMD_SUCCESS, a_pszArgv[ 0 ] );
	else
		logInfo( TOKEN_CMD_FAILED, a_pszArgv[ 0 ] );

	return rc;
}
