/*	$NetBSD: riscosdisk.c,v 1.1.36.1 2006/04/22 11:37:10 simonb Exp $	*/

/*-
 * Copyright (c) 2001, 2006 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/disklabel.h>
#include <lib/libsa/stand.h>
#include <riscoscalls.h>
#include <riscosdisk.h>
#include <riscospart.h>

#include <stdarg.h>

struct riscosdisk {
	void	*privword;
	int	drive;
	daddr_t	boff;
};

static void *
rodisk_getprivateword(char const *fsname)
{
	void *privword;
	char module[20];
	os_error *err;

	/* XXX The %c dance is because libsa printf can't do %% */
	snprintf(module, sizeof(module), "FileCore%c%s", '%', fsname);
	err = xosmodule_lookup(module, NULL, NULL, NULL, &privword, NULL);
	if (err != NULL)
		return NULL;
	return privword;
}

int
rodisk_open(struct open_file *f, ... /* char const *fsname, int drive,
    int partition */)
{
	va_list ap;
	struct disklabel dl;
	char const *fsname;
	int drive, partition, nfd, nhd, err;
	struct riscosdisk *rd;
	void *privword;

	va_start(ap, f);
	fsname = va_arg(ap, char const *);
	drive = va_arg(ap, int);
	partition = va_arg(ap, int);
	va_end(ap);

	if ((privword = rodisk_getprivateword(fsname)) == NULL)
		return ECTLR;
	if (xfilecore_drives(&privword, NULL, &nfd, &nhd) != NULL)
		return ECTLR;
	if (drive < 0 ||
	    (drive < 4 && drive >= nfd) ||
	    drive >= nhd + 4)
		return EUNIT;

	rd = alloc(sizeof(*rd));
	rd->privword = privword;
	rd->drive = drive;
	rd->boff = 0;
	f->f_devdata = rd;
	if (partition != RAW_PART) {
		err = getdisklabel_acorn(f, &dl);
		if (err != 0) {
			dealloc(rd, sizeof(*rd));
			return err;
		}
		if (partition >= dl.d_npartitions ||
		    dl.d_partitions[partition].p_size == 0) {
			dealloc(rd, sizeof(*rd));
			return EPART;
		}
		rd->boff = dl.d_partitions[partition].p_offset;
	}
	return 0;
}

int
rodisk_close(struct open_file *f)
{
	struct riscosdisk *rd = f->f_devdata;

	dealloc(rd, sizeof *rd);
	return 0;
}

int
rodisk_strategy(void *devdata, int flag, daddr_t dblk, size_t size,
    void *buf, size_t *rsize)
{
	struct riscosdisk *rd = devdata;
	size_t resid;
	uint32_t daddr, ndaddr;
	void *nbuf;
	os_error *err;

	if (flag != F_READ)
		return EROFS;

	dblk += rd->boff;

	if (rsize) *rsize = 0;
	if (dblk < 1 << (29 - DEV_BSHIFT)) {
		daddr = (dblk * DEV_BSIZE) | (rd->drive << 29);
		if ((err = xfilecorediscop_read_sectors(0, daddr, buf, size,
		    &rd->privword, &ndaddr, &nbuf, &resid)) != NULL)
			goto eio;
	} else if (dblk < 1 << 29) {
		daddr = dblk | (rd->drive << 29);
		if ((err = xfilecoresectorop_read_sectors(0, daddr, buf, size,
		    &rd->privword, &ndaddr, &nbuf, &resid)) != NULL)
			goto eio;
	} else if (dblk < 1LL << (64 - DEV_BSHIFT)){
		struct filecore_daddr64 daddr64;
		daddr64.drive = rd->drive;
		daddr64.daddr = dblk * DEV_BSIZE;
		if ((err = xfilecorediscop64_read_sectors(0, &daddr64, buf,
		    size, NULL, &rd->privword, &ndaddr, &nbuf, &resid)) !=
		    NULL)
			goto eio;
	} else
		return EIO;
	if (rsize)
		*rsize = size - resid;
	return 0;
eio:
	printf("Error: %s\n", err->errmess);
	return EIO;
}
