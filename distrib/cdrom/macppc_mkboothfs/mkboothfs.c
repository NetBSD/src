/*	$NetBSD: mkboothfs.c,v 1.2.4.1 2008/06/23 04:28:54 wrstuden Exp $	*/

/*-
 * Copyright (c) 2005, 2006 Izumi Tsutsui.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#include "../../sys/sys/bootblock.h"
#else
#include <sys/bootblock.h>
#endif

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BSIZE		512
#define BUFSIZE		(8 * 1024)

static void usage(void);

/*
 * Creates a file for use by mkisofs's -boot-hfs-file.
 */

int
main(int argc, char **argv)
{
	char *boothfs;
	int ofd;
	struct apple_drvr_map dm;
	struct apple_part_map_entry pme;
	char *buf;

	if (argc != 2)
		usage();

	boothfs = argv[1];

	buf = malloc(BUFSIZE);
	if (buf == NULL)
		err(1, "malloc write buffer");

	/* create output boot-hfs-file */
	if ((ofd = open(boothfs, O_CREAT | O_TRUNC | O_WRONLY, 0644)) == -1)
		err(1, "create output boot-hfs-file `%s'", boothfs);
	
	/*
	 * Populate 18 byte driver map header in the first 512 byte block
	 */
	memset(&dm, 0, sizeof dm);
	dm.sbSig = htobe16(APPLE_DRVR_MAP_MAGIC);
	dm.sbBlockSize = htobe16(2048);
	dm.sbBlkCount = htobe32(0);	/* XXX */
	dm.sbDevType = htobe16(1);
	dm.sbDevID = htobe16(1);
	dm.sbData = 0;
	dm.sbDrvrCount = 0;

	memset(buf, 0, BSIZE);
	memcpy(buf, &dm, sizeof(dm));
	write(ofd, buf, BSIZE);

	/*
	 * Write 2048-byte/sector map in the second 512 byte block
	 */
	memset(&pme, 0, sizeof(pme));
	pme.pmSig =		htobe16(APPLE_PART_MAP_ENTRY_MAGIC);
	pme.pmMapBlkCnt =	htobe32(1);
	pme.pmPyPartStart =	htobe32(1);
	pme.pmPartBlkCnt =	htobe32(1);
	pme.pmDataCnt =		htobe32(1);
	strlcpy(pme.pmPartName, "NetBSD_BootBlock", sizeof(pme.pmPartName));
	strlcpy(pme.pmPartType, "Apple_Driver", sizeof(pme.pmPartType));
	pme.pmPartStatus =	htobe32(0x3b);
	pme.pmBootSize =	htobe32(MACPPC_BOOT_BLOCK_MAX_SIZE);
	pme.pmBootLoad =	htobe32(0x4000);
	pme.pmBootEntry =	htobe32(0x4000);
	strlcpy(pme.pmProcessor, "PowerPC", sizeof(pme.pmProcessor));

	memset(buf, 0, BSIZE);
	memcpy(buf, &pme, sizeof(pme));
	write(ofd, buf, BSIZE);

	/*
	 * Write 512-byte/sector map in the third 512 byte block
	 */
	pme.pmPyPartStart =	htobe32(4);
	pme.pmPartBlkCnt =	htobe32(4);
	pme.pmDataCnt =		htobe32(4);
	memset(buf, 0, BSIZE);
	memcpy(buf, &pme, sizeof(pme));
	write(ofd, buf, BSIZE);

	/*
	 * Placeholder for 2048 byte padding
	 */
	memset(buf, 0, BSIZE);
	write(ofd, buf, BSIZE);

	/*
	 * Placeholder for NetBSD bootblock
	 */
	memset(buf, 0, MACPPC_BOOT_BLOCK_MAX_SIZE);
	write(ofd, buf, MACPPC_BOOT_BLOCK_MAX_SIZE);

	/*
	 * Prepare HFS "bootblock"; enough to pacify mkisofs.
	 */
	memset(buf, 0, BSIZE * 2);
	buf[0] = 0x4c;
	buf[1] = 0x4b;
	if (write(ofd, buf, BSIZE * 2) != BSIZE * 2)
		err(1, "write boot-hfs-file `%s'", boothfs);
 
	free(buf);
	close(ofd);
	return 0;
}

static void
usage(void)
{
	const char *prog;

	prog = getprogname();
	fprintf(stderr, "usage: %s boot-hfs-file\n", prog);
	exit(1);
}
