
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004, 2005
 *
 */


#include <stdio.h>
#include <string.h>

#include "trousers/tss.h"
#include "tsplog.h"

#ifdef TSS_DEBUG

/*
 * LogBlobData()
 *
 * Log a blob's data to the debugging stream
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
	int i;

	if (getenv("TSS_DEBUG_OFF"))
		return;

	memset(temp, 0, sizeof(temp));

	for (i = 0; (unsigned long)i < sizeOfBlob; i++) {
		if ((i > 0) && ((i % 16) == 0))	{
			fprintf(stdout, "%s\n", temp);
			memset(temp, 0, sizeof(temp));
		}
		snprintf(&temp[(i%16)*3], 4, "%.2X ", blob[i]);
	}
	fprintf(stdout, "%s\n", temp);
}

TSS_RESULT
LogTSPERR(TSS_RESULT result, char *file, int line)
{
	if (getenv("TSS_DEBUG_OFF") == NULL)
		fprintf(stderr, "%s %s %s:%d: 0x%x\n", "LOG_RETERR", APPID, file, line, result);

	return (result | TSS_LAYER_TSP);
}

#endif
