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

#ifndef __TPM_UTILS_H
#define __TPM_UTILS_H

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(String) gettext(String)
#define N_(String) String
#else
#define setlocale(category, local)
#define bindtextdomain(package, localedir)
#define textdomain(package);
#define _(String) String
#define N_(String) String
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
//#define GNU_SOURCE
#include <getopt.h>

#ifndef BOOL
#define BOOL int
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define CMD_VERSION             PACKAGE_VERSION

#define LOG_NONE                _("none")
#define LOG_LEVEL_NONE          0
#define LOG_ERROR               _("error")
#define LOG_LEVEL_ERROR         1
#define LOG_INFO                _("info")
#define LOG_LEVEL_INFO          2
#define LOG_DEBUG               _("debug")
#define LOG_LEVEL_DEBUG         3

#ifdef __GNUC__
#define __no_optimize __attribute__((optimize("O0")))
#else
#define __no_optimize
#endif

void * __no_optimize __memset(void *s, int c, size_t n);

typedef int (*CmdOptParser)( const int aOpt, const char *aOptArg );
typedef void (*CmdHelpFunction)( const char *aCmd );

void initIntlSys( );

int genericOptHandler( int a_iNumArgs, char **a_pszArgs,
		       const char *a_pszShortOpts,
		       struct option *a_psLongOpts, int a_iNumOpts,
		       CmdOptParser, CmdHelpFunction );
char *getPlainPasswd( const char* a_pszPrompt, BOOL a_bConfirm );
#ifdef TSS_LIB_IS_12
char *_getPasswd12( const char *a_pszPrompt, int *a_iLen, BOOL a_bConfirm, BOOL a_uUseUnicode);
char *getPasswd12( const char *a_pszPrompt, int *a_iLen, BOOL a_bConfirm );

#define _GETPASSWD	_getPasswd12
#define GETPASSWD	getPasswd12
#else
char *getPasswd( const char *a_pszPrompt, int *a_iLen, BOOL a_bConfirm );

#define _GETPASSWD	_getPasswd
#define GETPASSWD	getPasswd
#endif
char *_getPasswd( const char *a_pszPrompt, int *a_iLen, BOOL a_bConfirm, BOOL a_bUseUnicode);
void  shredPasswd( char *a_pszPasswd );
char *getReply( const char *a_pszPrompt, int a_iMaxLen );

extern int iLogLevel;
extern BOOL useUnicode;

int logHex( int a_iLen, void *a_pData );
int logMsg( const char *a_pszFormat, ... );
int logDebug( const char *a_pszFormat, ... );
int logInfo( const char *a_pszFormat, ... );
int logError( const char *a_pszFormat, ... );

int logProcess( FILE *a_pStream, const char *a_pszFormat, va_list a_vaArgs );
int logIt( FILE *a_pStream, const char *a_pszFormat, va_list a_vaArgs );

void  logSuccess( const char *a_pszCmd );
void  logCmdOption( const char *a_pszOption, const char *a_pszDescr );
void  logUnicodeCmdOption( );
void  logGenericOptions( );
void  logCmdHelp( const char *a_pszCmd );
void  logCmdHelpEx( const char *a_pszCmd, char *a_pszArgs[], char *a_pszArgDescs[] );
char *logBool( BOOL aValue );
void  logOwnerPassCmdOption( );
void  logNVIndexCmdOption( );

#endif
