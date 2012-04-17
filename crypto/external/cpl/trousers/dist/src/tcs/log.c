
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */


#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "trousers/tss.h"
#include "tcslog.h"

#ifdef TSS_DEBUG

/*
 * LogBlobData()
 *
 *   Log a blob's data to the debugging stream
 *
 * szDescriptor - The APPID tag found in the caller's environment at build time
 * sizeOfBlob - The size of the data to log
 * blob - the data to log
 *
 */

void
LogBlobData(char *szDescriptor, unsigned long sizeOfBlob, unsigned char *blob)
{
	char temp[64];
	unsigned int i;


	if (getenv("TCSD_FOREGROUND") == NULL)
		openlog(szDescriptor, LOG_NDELAY|LOG_PID, TSS_SYSLOG_LVL);
	memset(temp, 0, sizeof(temp));

	for (i = 0; (unsigned long)i < sizeOfBlob; i++) {
		if ((i > 0) && ((i % 16) == 0)) {
			if (getenv("TCSD_FOREGROUND") != NULL)
				fprintf(stdout, "%s %s\n", szDescriptor, temp);
			else
				syslog(LOG_DEBUG, "%s", temp);
			memset(temp, 0, sizeof(temp));
		}
		snprintf(&temp[(i%16)*3], 4, "%.2X ", blob[i]);
	}

	if (i == sizeOfBlob) {
		if (getenv("TCSD_FOREGROUND") != NULL)
			fprintf(stdout, "%s %s\n", szDescriptor, temp);
		else
			syslog(LOG_DEBUG, "%s", temp);
	}
}

void
LogTPMERR(TSS_RESULT result, char *file, int line)
{
	if (getenv("TSS_DEBUG_OFF") == NULL)
		fprintf(stderr, "%s %s %s:%d: 0x%x\n", "LOG_RETERR", "TPM", file, line, result);
}

TSS_RESULT
LogTDDLERR(TSS_RESULT result, char *file, int line)
{
	if (getenv("TSS_DEBUG_OFF") == NULL)
		fprintf(stderr, "%s %s %s:%d: 0x%x\n", "LOG_RETERR", APPID, file, line, result);

	return (result | TSS_LAYER_TDDL);
}

TSS_RESULT
LogTCSERR(TSS_RESULT result, char *file, int line)
{
	if (getenv("TSS_DEBUG_OFF") == NULL)
		fprintf(stderr, "%s %s %s:%d: 0x%x\n", "LOG_RETERR", APPID, file, line, result);

	return (result | TSS_LAYER_TCS);
}

#endif
