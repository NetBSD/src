/*	$NetBSD: pass1.c,v 1.1.2.3 2024/07/19 16:19:16 perseant Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <err.h>
#include <util.h>

#include <sys/errno.h>
#include <sys/param.h>
#include <sys/queue.h>

#define buf ubuf
#define vnode uvnode
#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_cksum.h>
#include <fs/exfatfs/exfatfs_conv.h>
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_inode.h>

#include "bufcache.h"
#include "vnode.h"
#include "fsck_exfatfs.h"
#include "pass1.h"
#include "extern.h"

extern int nvnodes;
extern unsigned long long total_files;
extern unsigned long long frag_files;
extern unsigned clusters_used;

#define HASH_BUCKETS 997 /* Essentially arbitrary */

struct validdata {
	struct exfatfs *fs;
	LIST_HEAD(dup, ino_entry) *duplist;
	uint8_t *bitmap;
};

/* Forward declarations */
static void hash_delete(void);
static int hash_insert(uint32_t);
static int hashfunc(uint32_t);
static void pass1_recursive(struct uvnode *, struct validdata *);
static unsigned validfunc(void *, struct xfinode *, off_t);

/*
 * Implement a set to store visited inode numbers.
 */
static LIST_HEAD(listhead, ino_entry) head[HASH_BUCKETS];

/* An arbitrary simple hash function */
static int
hashfunc(uint32_t cn)
{
	uint8_t *data = (uint8_t *)&cn;
	int r;

	r = exfatfs_cksum16(0, data, sizeof(cn), NULL, 0);
	r = r % HASH_BUCKETS;
	return r;
}

/* A list of file numbers that contain duplicate blocks */
static int on_duplist(struct dup *duplist, ino_t ino)
{
	struct ino_entry *tmp;
	
	LIST_FOREACH(tmp, duplist, list) {
		if (tmp->ino == ino)
			return EEXIST;
	}
	return 0;
}

/*
 * Insert this block into the set.  If it already exists,
 * return EEXIST.  Otherwise, return 0.
 */
static int
hash_insert(uint32_t cn)
{
	int hash = hashfunc(cn);
	struct ino_entry *tmp;
	
	/* Search for existing node */
	LIST_FOREACH(tmp, &head[hash], list)
		if (tmp->ino == cn)
			return EEXIST;

	/* Add it */
	tmp = ecalloc(1, sizeof(*tmp));
	tmp->ino = cn;
	LIST_INSERT_HEAD(&head[hash], tmp, list);

	return 0;
}

static void
hash_delete(void)
{
	int i;
	
	for (i = 0; i < HASH_BUCKETS; i++) {
		while (LIST_FIRST(&head[i]) != NULL)
			LIST_REMOVE(LIST_FIRST(&head[i]), list);
	}
}

/*
 * Follow FAT (or dead reckoning) recursively for every file
 * in the filesystem, starting with the root directory.
 * In the process we will discover bad/out of range FATs,
 * blocks that are double-allocated, and file streams that
 * appear in more than one place in the filesystem (identified
 * by starting block number).
 */
static void
pass1_recursive(struct uvnode *dvp, struct validdata *vdp)
{
	unsigned long dserial;
	assert(dvp->v_data != NULL);
	assert(VTOXI(dvp)->xi_serial > 0);
	assert(VTOXI(dvp)->xi_direntp[0] != NULL);
	assert(VTOXI(dvp)->xi_direntp[1] != NULL);
	dserial = VTOXI(dvp)->xi_serial;
	vref(dvp);
	exfatfs_scandir(dvp, 0, NULL, NULL, validfunc, vdp);
	assert(VTOXI(dvp)->xi_serial == dserial);
	vrele(dvp);
}

static unsigned
validfunc(void *arg, struct xfinode *xip, off_t unused)
{
	struct validdata *vdp = (struct validdata *)arg;
	struct exfatfs *fs = vdp->fs;
	struct uvnode *vp;
	struct ubuf *bp;
	uint32_t lcn, pcn, lcncount;
	int warned = 0, error;
	unsigned long dserial;

	dserial = xip->xi_serial;
	assert(dserial > 0);
	assert(xip->xi_direntp[0] != NULL);
	assert(xip->xi_direntp[1] != NULL);

	++total_files;

	if (debug)
		pwarn("Process file 0x%x datalength %lu\n", INUM(xip),
		      GET_DSE_DATALENGTH(xip));
	
	/* If this file has no file stream, skip it */
	if (GET_DSE_DATALENGTH(xip) == 0) {
		if (debug)
			pwarn("Skipping file 0x%x with no allocation\n",
			      INUM(xip));
		return 0;
	}

	assert(xip->xi_direntp[0] != NULL);
	assert(xip->xi_serial == dserial);

	/* Add to hash chain */
	if (hash_insert(GET_DSE_FIRSTCLUSTER(xip)) == EEXIST) {
		fprintf(stderr, "File 0x%lx startcluster 0x%x already found\n",
			(unsigned long)INUM(xip),
			(unsigned)GET_DSE_FIRSTCLUSTER(xip));
		/* XXX ask user to delete, or auto-delete */
		assert(xip->xi_serial == dserial);
		return 0;
	}

	if (!IS_DSE_NOFATCHAIN(xip))
		++frag_files;

	vp = vget3(fs, INUM(xip), xip);
	assert(xip->xi_serial == dserial);
	if (VTOXI(vp) != xip) { /* Already in cache */
		fprintf(stderr, "** freeing serial %lu in favor of %lu\n",
			xip->xi_serial, VTOXI(vp)->xi_serial);
		exfatfs_freexfinode(xip);
		xip = VTOXI(vp);
		dserial = xip->xi_serial;
		assert(dserial > 0);
	}
	vref(vp);

	assert(xip->xi_direntp[0] != NULL);
	assert(xip->xi_serial == dserial);

	/* Account bitmap blocks */
	lcncount = howmany(GET_DSE_DATALENGTH(xip), EXFATFS_CSIZE(fs)); /* Not always a cluster multiple! */
	lcn = 0;
	pcn = GET_DSE_FIRSTCLUSTER(xip);
	while (lcn < lcncount) {
		if (pcn < 2 || pcn > fs->xf_ClusterCount + 1) {
			fprintf(stderr, "File 0x%lx cluster 0x%x out of bounds\n",
				(unsigned long)INUM(xip), (unsigned)pcn);
			break;
		}

		assert(xip->xi_direntp[0] != NULL);
		if (!warned && isset(vdp->bitmap, pcn - 2)) {
			fprintf(stderr, "File 0x%lx dup cluster 0x%x\n",
				(unsigned long)INUM(xip), pcn);
			if (!on_duplist(vdp->duplist, INUM(xip))) {
				struct ino_entry *de = ecalloc(1, sizeof *de);
				de->ino = INUM(xip);
				LIST_INSERT_HEAD(vdp->duplist, de, list);
			}
			++warned;
			--clusters_used;
		}
		setbit(vdp->bitmap, pcn - 2);
		++clusters_used;

		/* Get next lcn and pcn */
		++lcn;
		if (IS_DSE_NOFATCHAIN(xip)) {
			/* Dead reckoning */
			pcn = GET_DSE_FIRSTCLUSTER(xip) + lcn;
		} else {
			/* Follow the FAT chain */
			error = bread(fs->xf_devvp,
				      EXFATFS_FATBLK(fs, pcn),
				      FATBSIZE(fs), 0, &bp);
			if (error) {
				warn("File 0x%lx", (unsigned long)INUM(xip));
				goto out;
			}
			pcn = ((uint32_t *)bp->b_data)[EXFATFS_FATOFF(pcn)];
			brelse(bp, 0);
		}	
	}
	assert(xip->xi_serial == dserial);
	assert(xip->xi_direntp[0] != NULL);

	/* If there was an allocation after the end, make a note of it */
	if (pcn != 0xFFFFFFFF && !IS_DSE_NOFATCHAIN(xip)) {
		fprintf(stderr, "File 0x%lx has allocation after end of file%s\n",
			(unsigned long)INUM(xip), (Pflag ? ", truncating" : ""));
		if (Pflag) {
			/* XXX Truncate the file */
		}
	}

	assert(xip->xi_serial == dserial);
	assert(xip->xi_direntp[0] != NULL);
	assert(xip->xi_direntp[1] != NULL);
	if (ISDIRECTORY(xip)) {
		if (debug) {
			uint16_t ucs2[NAME_MAX];
			uint8_t utf8[NAME_MAX];
			int len;
			memset(ucs2, 0, sizeof ucs2);
			memset(utf8, 0, sizeof utf8);
			exfatfs_get_file_name(xip, ucs2, &len, sizeof ucs2);
			exfatfs_ucs2utf8str(ucs2, len, utf8, NAME_MAX);
			printf("nv=%d scan directory 0x%lx, %s\n",
			       nvnodes, (unsigned long)INUM(xip), utf8);
			assert(xip->xi_serial == dserial);
		}
		pass1_recursive(vp, vdp);
	}

out:
	assert(xip->xi_serial == dserial);
	vrele(vp);
	return SCANDIR_DONTFREE;
}

void
pass1(struct exfatfs *fs, struct dup *duplist, uint8_t *bitmap)
{
	struct validdata vd;
	
	vd.fs = fs;
	vd.duplist = duplist;
	vd.bitmap = bitmap;

	if (!Pflag) {
		fprintf(stderr, "** Phase 1 - Scan for dups\n");
	}
	pass1_recursive(fs->xf_rootvp, &vd);
	
	/* Clean up */
	hash_delete();
}

