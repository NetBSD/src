/*	$NetBSD: iris_scsictl.c,v 1.1 2019/01/12 16:44:47 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Naruaki Etomi
 * All rights reserved.
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

/*
 * Silicon Graphics "IRIS" series MIPS processors machine bootloader.
 * Front end of SCSI commands driver.
 */

#include <lib/libsa/stand.h>

#include "iris_machdep.h"
#include "iris_scsivar.h"
#include "iris_scsicmd.h"

int
scsi_test_unit_rdy(void)
{
	struct wd33c93_softc *sc = &wd33c93_softc[scsi_ctlr];
	static struct scsi_cdb6 cdb = { CMD_TEST_UNIT_READY };

	return wd33c93_go(sc, (uint8_t *)&cdb, sizeof(cdb), NULL, NULL);
}

int
scsi_read_capacity(uint8_t *buf, size_t olen)
{
	size_t len = olen;
	size_t *lenp;
	lenp = &len;
	struct wd33c93_softc *sc = &wd33c93_softc[scsi_ctlr];
	static struct scsi_cdb10 cdb = { CMD_READ_CAPACITY };

	return wd33c93_go(sc, (uint8_t *)&cdb, sizeof(cdb), buf, lenp);
}

int
scsi_read(uint8_t *buf, size_t olen, daddr_t blk, u_int nblk)
{
	size_t len = olen;
	size_t *lenp;
	lenp = &len;
	struct wd33c93_softc *sc = &wd33c93_softc[scsi_ctlr];
	struct scsi_cdb10 cdb;

	memset(&cdb, 0, sizeof(cdb));
	cdb.cmd = CMD_READ_EXT;
	cdb.lbah = blk >> 24;
	cdb.lbahm = blk >> 16;
	cdb.lbalm = blk >> 8;
	cdb.lbal = blk;
	cdb.lenh = nblk >> (8 + DEV_BSHIFT);
	cdb.lenl = nblk >> DEV_BSHIFT;
	wd33c93_go(sc, (uint8_t *)&cdb, sizeof(cdb), buf, lenp);

	if (*lenp == olen)
		return 1;

	return 0;
}

int
scsi_write(uint8_t *buf, size_t olen, daddr_t blk, u_int nblk)
{
	size_t len = olen;
	size_t *lenp;
	lenp = &len;

	struct wd33c93_softc *sc = &wd33c93_softc[scsi_ctlr];
	struct scsi_cdb10 cdb;

	memset(&cdb, 0, sizeof(cdb));
	cdb.cmd = CMD_WRITE_EXT;
	cdb.lbah = blk >> 24;
	cdb.lbahm = blk >> 16;
	cdb.lbalm = blk >> 8;
	cdb.lbal = blk;
	cdb.lenh = nblk >> (8 + DEV_BSHIFT);
	cdb.lenl = nblk >> DEV_BSHIFT;
	wd33c93_go(sc, (uint8_t *)&cdb, sizeof(cdb), buf, lenp);

	if (*lenp == olen)
		return 1;

	return 0;
}
