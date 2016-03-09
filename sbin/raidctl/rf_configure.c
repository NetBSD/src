/*	$NetBSD: rf_configure.c,v 1.26 2016/03/09 19:53:32 christos Exp $	*/

/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Mark Holland
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/***************************************************************
 *
 * rf_configure.c -- code related to configuring the raidframe system
 *
 * configuration is complicated by the fact that we want the same
 * driver to work both in the kernel and at user level.  In the
 * kernel, we can't read the configuration file, so we configure
 * by running a user-level program that reads the config file,
 * creates a data structure describing the configuration and
 * passes it into the kernel via an ioctl.  Since we want the config
 * code to be common between the two versions of the driver, we
 * configure using the same two-step process when running at
 * user level.  Of course, at user level, the config structure is
 * passed directly to the config routine, rather than via ioctl.
 *
 * This file is not compiled into the kernel, so we have no
 * need for KERNEL ifdefs.
 *
 **************************************************************/
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: rf_configure.c,v 1.26 2016/03/09 19:53:32 christos Exp $");
#endif


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <dev/raidframe/raidframevar.h>
#include <dev/raidframe/raidframeio.h>
#include "rf_configure.h"

RF_LayoutSW_t *rf_GetLayout(RF_ParityConfig_t parityConfig);
char   *rf_find_non_white(char *p);
char   *rf_find_white(char *p);
#define RF_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define RF_ERRORMSG(s)            printf((s))
#define RF_ERRORMSG1(s,a)         printf((s),(a))
#define RF_ERRORMSG2(s,a,b)       printf((s),(a),(b))

int     distSpareYes = 1;
int     distSpareNo = 0;

/* The mapsw[] table below contains all the various RAID types that might
be supported by the kernel.  The actual supported types are found
in sys/dev/raidframe/rf_layout.c. */

static RF_LayoutSW_t mapsw[] = {
	/* parity declustering */
	{'T', "Parity declustering",
	 rf_MakeLayoutSpecificDeclustered, &distSpareNo},
	/* parity declustering with distributed sparing */
	{'D', "Distributed sparing parity declustering",
	 rf_MakeLayoutSpecificDeclustered, &distSpareYes},
	/* declustered P+Q */
	{'Q', "Declustered P+Q",
	 rf_MakeLayoutSpecificDeclustered, &distSpareNo},
	/* RAID 5 with rotated sparing */
	{'R', "RAID Level 5 rotated sparing", rf_MakeLayoutSpecificNULL, NULL},
	/* Chained Declustering */
	{'C', "Chained Declustering", rf_MakeLayoutSpecificNULL, NULL},
	/* Interleaved Declustering */
	{'I', "Interleaved Declustering", rf_MakeLayoutSpecificNULL, NULL},
	/* RAID level 0 */
	{'0', "RAID Level 0", rf_MakeLayoutSpecificNULL, NULL},
	/* RAID level 1 */
	{'1', "RAID Level 1", rf_MakeLayoutSpecificNULL, NULL},
	/* RAID level 4 */
	{'4', "RAID Level 4", rf_MakeLayoutSpecificNULL, NULL},
	/* RAID level 5 */
	{'5', "RAID Level 5", rf_MakeLayoutSpecificNULL, NULL},
	/* Evenodd */
	{'E', "EvenOdd", rf_MakeLayoutSpecificNULL, NULL},
	/* Declustered Evenodd */
	{'e', "Declustered EvenOdd",
	 rf_MakeLayoutSpecificDeclustered, &distSpareNo},
	/* parity logging */
	{'L', "Parity logging", rf_MakeLayoutSpecificNULL, NULL},
	/* end-of-list marker */
	{'\0', NULL, NULL, NULL}
};
RF_LayoutSW_t *
rf_GetLayout(RF_ParityConfig_t parityConfig)
{
	RF_LayoutSW_t *p;

	/* look up the specific layout */
	for (p = &mapsw[0]; p->parityConfig; p++)
		if (p->parityConfig == parityConfig)
			break;
	if (!p->parityConfig)
		return (NULL);
	return (p);
}

static int rf_search_file_for_start_of(const char *string, char *buf,
    int len, FILE * fp);
static int rf_get_next_nonblank_line(char *buf, int len, FILE * fp,
    const char *errmsg);

/*
 * called from user level to read the configuration file and create
 * a configuration control structure.  This is used in the user-level
 * version of the driver, and in the user-level program that configures
 * the system via ioctl.
 */
int 
rf_MakeConfig(char *configname, RF_Config_t *cfgPtr)
{
	int numscanned, val, r, c, retcode, aa, bb, cc;
	char buf[256], buf1[256], *cp;
	RF_LayoutSW_t *lp;
	FILE *fp;

	bzero((char *) cfgPtr, sizeof(RF_Config_t));

	fp = fopen(configname, "r");
	if (!fp) {
		printf("Can't open config file %s\n", configname);
		return (-1);
	}
	rewind(fp);
	if (rf_search_file_for_start_of("array", buf, 256, fp)) {
		printf("Unable to find start of \"array\" params in config file %s\n", configname);
		retcode = -1;
		goto out;
	}
	rf_get_next_nonblank_line(buf, 256, fp, "Config file error (\"array\" section):  unable to get numRow and numCol\n");

	/*
         * wackiness with aa, bb, cc to get around size problems on
         * different platforms
         */
	numscanned = sscanf(buf, "%d %d %d", &aa, &bb, &cc);
	if (numscanned != 3) {
		printf("Config file error (\"array\" section):  unable to get numRow, numCol, numSpare\n");
		retcode = -1;
		goto out;
	}
	cfgPtr->numRow = (RF_RowCol_t) aa;
	cfgPtr->numCol = (RF_RowCol_t) bb;
	cfgPtr->numSpare = (RF_RowCol_t) cc;

	/* debug section is optional */
	for (c = 0; c < RF_MAXDBGV; c++)
		cfgPtr->debugVars[c][0] = '\0';
	rewind(fp);
	if (!rf_search_file_for_start_of("debug", buf, 256, fp)) {
		for (c = 0; c < RF_MAXDBGV; c++) {
			if (rf_get_next_nonblank_line(buf, 256, fp, NULL))
				break;
			cp = rf_find_non_white(buf);
			if (!strncmp(cp, "START", strlen("START")))
				break;
			(void) strlcpy(&cfgPtr->debugVars[c][0], cp,
			    sizeof(cfgPtr->debugVars[c]));
		}
	}
	rewind(fp);
	strlcpy(cfgPtr->diskQueueType, "fifo", sizeof(cfgPtr->diskQueueType));
	cfgPtr->maxOutstandingDiskReqs = 1;
	/* scan the file for the block related to disk queues */
	if (rf_search_file_for_start_of("queue", buf, 256, fp)) {
		RF_ERRORMSG2("[No disk queue discipline specified in config file %s.  Using %s.]\n", configname, cfgPtr->diskQueueType);
	} else {
		if (rf_get_next_nonblank_line(buf, 256, fp, NULL)) {
			RF_ERRORMSG2("[No disk queue discipline specified in config file %s.  Using %s.]\n", configname, cfgPtr->diskQueueType);
		}
	}

	/* the queue specifier line contains two entries: 1st char of first
	 * word specifies queue to be used 2nd word specifies max num reqs
	 * that can be outstanding on the disk itself (typically 1) */
	if (sscanf(buf, "%255s %d", buf1, &val) != 2) {
		RF_ERRORMSG1("Can't determine queue type and/or max outstanding reqs from line: %s", buf);
		RF_ERRORMSG2("Using %s-%d\n", cfgPtr->diskQueueType, cfgPtr->maxOutstandingDiskReqs);
	} else {
		char *ch;
		bcopy(buf1, cfgPtr->diskQueueType,
		    RF_MIN(sizeof(cfgPtr->diskQueueType), strlen(buf1) + 1));
		for (ch = buf1; *ch; ch++) {
			if (*ch == ' ') {
				*ch = '\0';
				break;
			}
		}
		cfgPtr->maxOutstandingDiskReqs = val;
	}

	rewind(fp);

	if (rf_search_file_for_start_of("disks", buf, 256, fp)) {
		RF_ERRORMSG1("Can't find \"disks\" section in config file %s\n", configname);
		retcode = -1;
		goto out;
	}
	for (r = 0; r < cfgPtr->numRow; r++) {
		for (c = 0; c < cfgPtr->numCol; c++) {
			if (rf_get_next_nonblank_line(
			    &cfgPtr->devnames[r][c][0], 50, fp, NULL)) {
				RF_ERRORMSG2("Config file error: unable to get device file for disk at row %d col %d\n", r, c);
				retcode = -1;
				goto out;
			}
		}
	}

	/* "spare" section is optional */
	rewind(fp);
	if (rf_search_file_for_start_of("spare", buf, 256, fp))
		cfgPtr->numSpare = 0;
	for (c = 0; c < cfgPtr->numSpare; c++) {
		if (rf_get_next_nonblank_line(&cfgPtr->spare_names[c][0],
		    256, fp, NULL)) {
			RF_ERRORMSG1("Config file error: unable to get device file for spare disk %d\n", c);
			retcode = -1;
			goto out;
		}
	}

	/* scan the file for the block related to layout */
	rewind(fp);
	if (rf_search_file_for_start_of("layout", buf, 256, fp)) {
		RF_ERRORMSG1("Can't find \"layout\" section in configuration file %s\n", configname);
		retcode = -1;
		goto out;
	}
	if (rf_get_next_nonblank_line(buf, 256, fp, NULL)) {
		RF_ERRORMSG("Config file error (\"layout\" section): unable to find common layout param line\n");
		retcode = -1;
		goto out;
	}
	c = sscanf(buf, "%d %d %d %c", &aa, &bb, &cc, &cfgPtr->parityConfig);
	cfgPtr->sectPerSU = (RF_SectorNum_t) aa;
	cfgPtr->SUsPerPU = (RF_StripeNum_t) bb;
	cfgPtr->SUsPerRU = (RF_StripeNum_t) cc;
	if (c != 4) {
		RF_ERRORMSG("Unable to scan common layout line\n");
		retcode = -1;
		goto out;
	}
	lp = rf_GetLayout(cfgPtr->parityConfig);
	if (lp == NULL) {
		RF_ERRORMSG1("Unknown parity config '%c'\n",
		    cfgPtr->parityConfig);
		retcode = -1;
		goto out;
	}

	retcode = lp->MakeLayoutSpecific(fp, cfgPtr, lp->makeLayoutSpecificArg);
out:
	fclose(fp);
	if (retcode < 0)
		retcode = errno = EINVAL;
	else
		errno = retcode;
	return (retcode);
}


/* used in architectures such as RAID0 where there is no layout-specific
 * information to be passed into the configuration code.
 */
int 
rf_MakeLayoutSpecificNULL(FILE *fp, RF_Config_t *cfgPtr, void *ignored)
{
	cfgPtr->layoutSpecificSize = 0;
	cfgPtr->layoutSpecific = NULL;
	return (0);
}

int 
rf_MakeLayoutSpecificDeclustered(FILE *configfp, RF_Config_t *cfgPtr, void *arg)
{
	int b, v, k, r, lambda, norotate, i, val, distSpare;
	char *cfgBuf, *bdfile, *p, *smname;
	char buf[256], smbuf[256];
	FILE *fp;

	distSpare = *((int *) arg);

	/* get the block design file name */
	if (rf_get_next_nonblank_line(buf, 256, configfp,
	    "Can't find block design file name in config file\n"))
		return (EINVAL);
	bdfile = rf_find_non_white(buf);
	if (bdfile[strlen(bdfile) - 1] == '\n') {
		/* strip newline char */
		bdfile[strlen(bdfile) - 1] = '\0';
	}
	/* open bd file, check validity of configuration */
	if ((fp = fopen(bdfile, "r")) == NULL) {
		RF_ERRORMSG1("RAID: config error: Can't open layout table file %s\n", bdfile);
		return (EINVAL);
	}
	if (fgets(buf, 256, fp) == NULL) {
		RF_ERRORMSG1("RAID: config error: Can't read layout from layout table file %s\n", bdfile);
		fclose(fp);
		return (EINVAL);
	}
	i = sscanf(buf, "%u %u %u %u %u %u", &b, &v, &k, &r, &lambda, &norotate);
	if (i == 5)
		norotate = 0;	/* no-rotate flag is optional */
	else if (i != 6) {
		RF_ERRORMSG("Unable to parse header line in block design file\n");
		fclose(fp);
		return (EINVAL);
	}
	/* set the sparemap directory.  In the in-kernel version, there's a
	 * daemon that's responsible for finding the sparemaps */
	if (distSpare) {
		if (rf_get_next_nonblank_line(smbuf, 256, configfp,
		    "Can't find sparemap file name in config file\n")) {
			fclose(fp);
			return (EINVAL);
		}
		smname = rf_find_non_white(smbuf);
		if (smname[strlen(smname) - 1] == '\n') {
			/* strip newline char */
			smname[strlen(smname) - 1] = '\0';
		}
	} else {
		smbuf[0] = '\0';
		smname = smbuf;
	}

	/* allocate a buffer to hold the configuration info */
	cfgPtr->layoutSpecificSize = RF_SPAREMAP_NAME_LEN +
	    6 * sizeof(int) + b * k;

	cfgBuf = (char *) malloc(cfgPtr->layoutSpecificSize);
	if (cfgBuf == NULL) {
		fclose(fp);
		return (ENOMEM);
	}
	cfgPtr->layoutSpecific = (void *) cfgBuf;
	p = cfgBuf;

	/* install name of sparemap file */
	for (i = 0; smname[i]; i++)
		*p++ = smname[i];
	/* pad with zeros */
	while (i < RF_SPAREMAP_NAME_LEN) {
		*p++ = '\0';
		i++;
	}

	/*
         * fill in the buffer with the block design parameters
         * and then the block design itself
         */
	*((int *) p) = b;
	p += sizeof(int);
	*((int *) p) = v;
	p += sizeof(int);
	*((int *) p) = k;
	p += sizeof(int);
	*((int *) p) = r;
	p += sizeof(int);
	*((int *) p) = lambda;
	p += sizeof(int);
	*((int *) p) = norotate;
	p += sizeof(int);

	while (fscanf(fp, "%d", &val) == 1)
		*p++ = (char) val;
	fclose(fp);
	if ((unsigned int)(p - cfgBuf) != cfgPtr->layoutSpecificSize) {
		RF_ERRORMSG2("Size mismatch creating layout specific data: is %d sb %d bytes\n", (int) (p - cfgBuf), (int) (6 * sizeof(int) + b * k));
		return (EINVAL);
	}
	return (0);
}

/****************************************************************************
 *
 * utilities
 *
 ***************************************************************************/

/* finds a non-white character in the line */
char *
rf_find_non_white(char *p)
{
	for (; *p != '\0' && (*p == ' ' || *p == '\t'); p++);
	return (p);
}

/* finds a white character in the line */
char *
rf_find_white(char *p)
{
	for (; *p != '\0' && (*p != ' ' && *p != '\t'); p++);
	return (p);
}

/*
 * searches a file for a line that says "START string", where string is
 * specified as a parameter
 */
static int 
rf_search_file_for_start_of(const char *string, char *buf, int len, FILE *fp)
{
	char *p;

	while (1) {
		if (fgets(buf, len, fp) == NULL)
			return (-1);
		p = rf_find_non_white(buf);
		if (!strncmp(p, "START", strlen("START"))) {
			p = rf_find_white(p);
			p = rf_find_non_white(p);
			if (!strncmp(p, string, strlen(string)))
				return (0);
		}
	}
}

/* reads from file fp into buf until it finds an interesting line */
int 
rf_get_next_nonblank_line(char *buf, int len, FILE *fp, const char *errmsg)
{
	char *p;
	int l;

	while (fgets(buf, len, fp) != NULL) {
		p = rf_find_non_white(buf);
		if (*p == '\n' || *p == '\0' || *p == '#')
			continue;
		l = strlen(buf)-1;
		while (l>=0 && (buf[l]==' ' || buf[l]=='\n')) {
			buf[l]='\0';
			l--;
		}
		return (0);
	}
	if (errmsg)
		RF_ERRORMSG1("%s", errmsg);
	return (1);
}

/*
 * Allocates an array for the spare table, and initializes it from a file.
 * In the user-level version, this is called when recon is initiated.
 * When/if I move recon into the kernel, there'll be a daemon that does
 * an ioctl into raidframe which will block until a spare table is needed.
 * When it returns, it will read a spare table from the file system,
 * pass it into the kernel via a different ioctl, and then block again
 * on the original ioctl.
 *
 * This is specific to the declustered layout, but doesn't belong in
 * rf_decluster.c because it uses stuff that can't be compiled into
 * the kernel, and it needs to be compiled into the user-level sparemap daemon.
 *
 */
void *
rf_ReadSpareTable(RF_SparetWait_t *req, char *fname)
{
	int i, j, numFound, linecount, tableNum, tupleNum,
	    spareDisk, spareBlkOffset;
	char buf[1024], targString[100], errString[100];
	RF_SpareTableEntry_t **table;
	FILE *fp = NULL;

	/* allocate and initialize the table */
	table = calloc(req->TablesPerSpareRegion, sizeof(*table));
	if (table == NULL) {
		warn("%s: Unable to allocate table", __func__);
		return NULL;
	}
	for (i = 0; i < req->TablesPerSpareRegion; i++) {
		table[i] = calloc(req->BlocksPerTable, sizeof(**table));
		if (table[i] == NULL) {
			warn("%s: Unable to allocate table", __func__);
			goto out;
		}
		for (j = 0; j < req->BlocksPerTable; j++)
			table[i][j].spareDisk =
			    table[i][j].spareBlockOffsetInSUs = -1;
	}

	/* 2.  open sparemap file, sanity check */
	if ((fp = fopen(fname, "r")) == NULL) {
		warn("%s: Can't open sparemap file %s", __func__, fname);
		goto out;
	}
	if (rf_get_next_nonblank_line(buf, 1024, fp,
	    "Invalid sparemap file:  can't find header line\n"))
		goto out;

	size_t len = strlen(buf);
	if (len != 0 && buf[len - 1] == '\n')
		buf[len - 1] = '\0';

	snprintf(targString, sizeof(targString), "fdisk %d\n", req->fcol);
	snprintf(errString, sizeof(errString),
	    "Invalid sparemap file:  can't find \"fdisk %d\" line\n",
	    req->fcol);
	for (;;) {
		rf_get_next_nonblank_line(buf, 1024, fp, errString);
		if (!strncmp(buf, targString, strlen(targString)))
			break;
	}

	/* no more blank lines or comments allowed now */
	linecount = req->TablesPerSpareRegion * req->TableDepthInPUs;
	for (i = 0; i < linecount; i++) {
		numFound = fscanf(fp, " %d %d %d %d", &tableNum, &tupleNum,
		    &spareDisk, &spareBlkOffset);
		if (numFound != 4) {
			warnx("Sparemap file prematurely exhausted after %d "
			    "of %d lines", i, linecount);
			goto out;
		}

		table[tableNum][tupleNum].spareDisk = spareDisk;
		table[tableNum][tupleNum].spareBlockOffsetInSUs =
		    spareBlkOffset * req->SUsPerPU;
	}

	fclose(fp);
	return ((void *) table);
out:
	if (fp)
		fclose(fp);
	for (i = 0; i < req->TablesPerSpareRegion; i++)
		free(table[i]);
	free(table);
	return NULL;
}
