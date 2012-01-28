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

#include "tpm_utils.h"

int iLogLevel = LOG_LEVEL_ERROR;

int logHex(int a_iLen, void *a_pData)
{

	int i, iByte;
	char *pData = a_pData;

	for (i = 0; i < a_iLen; i++) {
		if ((i % 32) == 0) {
			if (a_iLen > 32) {
				logMsg("\n\t");
			}
		} else if ((i % 4) == 0) {
			logMsg(" ");
		}

		iByte = pData[i];
		iByte &= 0x000000ff;
		logMsg("%02x", iByte);
	}

	logMsg("\n");

	return a_iLen;
}

int logMsg(const char *a_szFormat, ...)
{

	int iCount;
	va_list vaArgs;

	va_start(vaArgs, a_szFormat);
	iCount = logIt(stdout, a_szFormat, vaArgs);
	va_end(vaArgs);

	return iCount;
}

int logDebug(const char *a_szFormat, ...)
{

	int iCount;
	va_list vaArgs;

	if (iLogLevel < LOG_LEVEL_DEBUG)
		return 0;

	va_start(vaArgs, a_szFormat);
	iCount = logProcess(stdout, a_szFormat, vaArgs);
	va_end(vaArgs);

	return iCount;
}

int logInfo(const char *a_szFormat, ...)
{

	int iCount;
	va_list vaArgs;

	if (iLogLevel < LOG_LEVEL_INFO)
		return 0;

	va_start(vaArgs, a_szFormat);
	iCount = logProcess(stdout, a_szFormat, vaArgs);
	va_end(vaArgs);

	return iCount;
}

int logError(const char *a_szFormat, ...)
{

	int iCount;
	va_list vaArgs;

	if (iLogLevel < LOG_LEVEL_ERROR)
		return 0;

	va_start(vaArgs, a_szFormat);
	iCount = logProcess(stderr, a_szFormat, vaArgs);
	va_end(vaArgs);

	return iCount;
}

int logProcess(FILE * a_sStream, const char *a_szFormat, va_list a_vaArgs)
{

	return logIt(a_sStream, a_szFormat, a_vaArgs);
}

int logIt(FILE * a_sStream, const char *a_szFormat, va_list a_vaArgs)
{

	return vfprintf(a_sStream, a_szFormat, a_vaArgs);
}

void logSuccess(const char *a_cmd)
{
	logInfo(_("%s succeeded\n"), a_cmd);
}

void logCmdOption(const char *aOption, const char *aDescr)
{
	logMsg("\t%s\n\t\t%s\n", aOption, aDescr);
}

void logGenericOptions()
{
	char *lOpt = NULL;

	lOpt = malloc(16+strlen(LOG_NONE)+strlen(LOG_ERROR)+
			strlen(LOG_INFO)+strlen(LOG_DEBUG));
	if ( lOpt )
		sprintf( lOpt, "-l, --log [%s|%s|%s|%s]", LOG_NONE, LOG_ERROR, LOG_INFO, LOG_DEBUG );

	logCmdOption("-h, --help", _("Display command usage info."));
	logCmdOption("-v, --version", _("Display command version info."));
	logCmdOption( lOpt, _("Set logging level."));
	free ( lOpt );
}

void logUnicodeCmdOption()
{
	logCmdOption("-u, --unicode", _("Use TSS UNICODE encoding for passwords to comply with applications using TSS popup boxes"));
}

void logOwnerPassCmdOption()
{
	logCmdOption("-o, --pwdo", _("Owner password"));
}

void logNVIndexCmdOption()
{
	logCmdOption("-i, --index", _("Index of the NVRAM area"));
}

void logCmdHelp(const char *aCmd)
{
	logMsg(_("Usage: %s [options]\n"), aCmd);
	logGenericOptions();
}

void logCmdHelpEx(const char *aCmd, char *aArgs[], char *aArgDescs[])
{
	int i;

	logMsg(_("Usage: %s [options]"), aCmd);
	for (i = 0; aArgs[i]; i++)
		logMsg(" %s", aArgs[i]);
	logMsg("\n");
	for (i = 0; aArgDescs[i]; i++)
		logMsg("%s\n", aArgDescs[i]);
	logGenericOptions();
}

char *logBool(BOOL aValue)
{
	return aValue ? _("true") : _("false");
}
