/*	$NetBSD: conf.c,v 1.17.8.1 2018/05/02 07:20:05 pgoyette Exp $ */
/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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

 /* All bugs are subject to removal without further notice */

#include <sys/param.h>

#include <netinet/in.h>

#include "../../include/rpb.h"

#include <lib/libkern/libkern.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/ufs.h>
#include <lib/libsa/nfs.h>
#include <lib/libsa/cd9660.h>
#include <lib/libsa/ustarfs.h>

#include "vaxstand.h"

static int nostrategy(void *, int, daddr_t, size_t, void *, size_t *);

struct	devsw devsw[]={
	SADEV("hp",hpstrategy, hpopen, nullsys, noioctl),
	SADEV("qe",nostrategy, qeopen, qeclose, noioctl), /* DEQNA */
	SADEV("ctu",ctustrategy, ctuopen, nullsys, noioctl),
	SADEV("ra",rastrategy, raopen, nullsys, noioctl),
	SADEV("mt",rastrategy, raopen, nullsys, noioctl),
        SADEV("rom",romstrategy, romopen, nullsys, noioctl),
        SADEV("rd",mfmstrategy, mfmopen, nullsys, noioctl),
        SADEV("sd",romstrategy, romopen, nullsys, noioctl),
        SADEV("sd",romstrategy, romopen, nullsys, noioctl),
	SADEV("st",nullsys, nullsys, nullsys, noioctl),
	SADEV("le",nostrategy, leopen, leclose, noioctl), /* LANCE */
        SADEV("ze",nostrategy, zeopen, zeclose, noioctl), /* SGEC */
	SADEV("rl",romstrategy, romopen, nullsys, noioctl),
	SADEV("de",nostrategy, deopen, declose, noioctl), /* DEUNA */
	SADEV("ni",nostrategy, niopen, nullsys, noioctl), /* DEBNA */
};

int	cnvtab[] = {
	BDEV_HP,
	BDEV_QE,
	BDEV_CNSL,
	BDEV_UDA,
	BDEV_TK,
	-1,
	BDEV_RD,
	BDEV_SD,
	BDEV_SDN,
	BDEV_ST,
	BDEV_LE,
	BDEV_ZE,
	BDEV_RL,
	BDEV_DE,
	BDEV_NI,
};

int     ndevs = (sizeof(devsw)/sizeof(devsw[0]));

struct fs_ops file_system[] = {
	FS_OPS(ffsv1),
	FS_OPS(ffsv2),
	FS_OPS(nfs),
	FS_OPS(cd9660),
	FS_OPS(ustarfs),
};

int nfsys = __arraycount(file_system);

int
nostrategy(void *f, int func, daddr_t dblk,
    size_t size, void *buf, size_t *rsize)
{
	*rsize = size;
	memset(buf, 0, size);
	return 0;
}
