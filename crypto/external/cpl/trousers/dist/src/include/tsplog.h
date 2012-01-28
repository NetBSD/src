
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */


#ifndef _TSPLOG_H_
#define _TSPLOG_H_

#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>

/* Debug logging */
#ifdef TSS_DEBUG
/* log to stdout */
#define LogMessage(dest, priority, layer, fmt, ...) \
	do { \
		if (getenv("TSS_DEBUG_OFF") == NULL) { \
			fprintf(dest, "%s %s %s:%d " fmt "\n", priority, layer, __FILE__, __LINE__, ## __VA_ARGS__); \
		} \
	} while (0)

#define LogDebug(fmt, ...)	LogMessage(stdout, "LOG_DEBUG", APPID, fmt, ##__VA_ARGS__)
#define LogDebugFn(fmt, ...)	LogMessage(stdout, "LOG_DEBUG", APPID, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LogDebugData(sz,blb)	LogBlobData(APPID, sz, blb)

/* Error logging */
#define LogError(fmt, ...)	LogMessage(stderr, "LOG_ERR", APPID, "ERROR: " fmt, ##__VA_ARGS__)
/* Warn logging */
#define LogWarn(fmt, ...)	LogMessage(stdout, "LOG_WARNING", APPID, "WARNING: " fmt, ##__VA_ARGS__)
/* Info Logging */
#define LogInfo(fmt, ...)	LogMessage(stdout, "LOG_INFO", APPID, fmt, ##__VA_ARGS__)
/* Return Value logging */
extern TSS_RESULT LogTSPERR(TSS_RESULT, char *, int);
#else
#define LogDebug(fmt, ...)
#define LogDebugFn(fmt, ...)
#define LogDebugData(sz,blb)
#define LogError(fmt, ...)
#define LogWarn(fmt, ...)
#define LogInfo(fmt, ...)
#endif

void LogBlobData(char *appid, unsigned long sizeOfBlob, unsigned char *blob);

#endif
