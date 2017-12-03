/*	$NetBSD: dkwedge_rdb.c,v 1.4.18.2 2017/12/03 11:37:00 jdolecek Exp $	*/

/*
 * Adapted from arch/amiga/amiga/disksubr.c:
 *
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_disksubr.c	7.16 (Berkeley) 5/4/91
 */

/*
 * Copyright (c) 1994 Christian E. Hopps
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_disksubr.c	7.16 (Berkeley) 5/4/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dkwedge_rdb.c,v 1.4.18.2 2017/12/03 11:37:00 jdolecek Exp $");

#include <sys/param.h>
#include <sys/disklabel_rdb.h>
#include <sys/disk.h>
#include <sys/endian.h>
#include <sys/malloc.h>
#ifdef _KERNEL
#include <sys/systm.h>
#endif

/*
 * In /usr/src/sys/dev/scsipi/sd.c, routine sdstart() adjusts the
 * block numbers, it changes from DEV_BSIZE units to physical units:
 * blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
 * As long as media with sector sizes of 512 bytes are used, this
 * doesn't matter (divide by 1), but for successful usage of media with
 * greater sector sizes (e.g. 640MB MO-media with 2048 bytes/sector)
 * we must multiply block numbers with (lp->d_secsize / DEV_BSIZE)
 * to keep "unchanged" physical block numbers.
 */
#define	SD_C_ADJUSTS_NR
#ifdef SD_C_ADJUSTS_NR
#define	ADJUST_NR(x)	((x) * (secsize / DEV_BSIZE))
#else
#define	ADJUST_NR(x)	(x)
#endif

#ifdef _KERNEL
#define	DKW_MALLOC(SZ)	malloc((SZ), M_DEVBUF, M_WAITOK)
#define	DKW_FREE(PTR)	free((PTR), M_DEVBUF)
#define	DKW_REALLOC(PTR, NEWSZ)	realloc((PTR), (NEWSZ), M_DEVBUF, M_WAITOK)
#else
#define	DKW_MALLOC(SZ)	malloc((SZ))
#define	DKW_FREE(PTR)	free((PTR))
#define	DKW_REALLOC(PTR, NEWSZ)	realloc((PTR), (NEWSZ))
#endif

static unsigned rdbchksum(void *);
static unsigned char getarchtype(unsigned);
static const char *archtype_to_ptype(unsigned char);

static int
dkwedge_discover_rdb(struct disk *pdk, struct vnode *vp)
{
	struct dkwedge_info dkw;
	struct partblock *pbp;
	struct rdblock *rbp;
	void *bp;
	int error;
	unsigned blk_per_cyl, bufsize, newsecsize, nextb, secsize, tabsize;
	const char *ptype;
	unsigned char archtype;
	bool found, root, swap;

	secsize = DEV_BSIZE << pdk->dk_blkshift;
	bufsize = roundup(MAX(sizeof(struct partblock), sizeof(struct rdblock)),
	    secsize);
	bp = DKW_MALLOC(bufsize);

	/*
	 * find the RDB block
	 * XXX bsdlabel should be detected by the other method
	 */
	for (nextb = 0; nextb < RDB_MAXBLOCKS; nextb++) {
		error = dkwedge_read(pdk, vp, ADJUST_NR(nextb), bp, bufsize);
		if (error) {
			aprint_error("%s: unable to read RDB @ %u, "
			    "error = %d\n", pdk->dk_name, nextb, error);
			error = ESRCH;
			goto done;
		}
		rbp = (struct rdblock *)bp;
		if (be32toh(rbp->id) == RDBLOCK_ID) {
			if (rdbchksum(rbp) == 0)
				break;
			else
				aprint_error("%s: RDB bad checksum @ %u\n",
				    pdk->dk_name, nextb);
		}
	}

	if (nextb == RDB_MAXBLOCKS) {
		error = ESRCH;
		goto done;
	}

	newsecsize = be32toh(rbp->nbytes);
	if (secsize != newsecsize) {
		aprint_verbose("secsize changed from %u to %u\n",
		    secsize, newsecsize);
		secsize = newsecsize;
		bufsize = roundup(MAX(sizeof(struct partblock),
		    sizeof(struct rdblock)), secsize);
		bp = DKW_REALLOC(bp, bufsize);
	}

	strlcpy(dkw.dkw_parent, pdk->dk_name, sizeof(dkw.dkw_parent));

	found = root = swap = false;
	/*
	 * scan for partition blocks
	 */
	for (nextb = be32toh(rbp->partbhead); nextb != RDBNULL;
	     nextb = be32toh(pbp->next)) {
		error = dkwedge_read(pdk, vp, ADJUST_NR(nextb), bp, bufsize);
		if (error) {
			aprint_error("%s: unable to read RDB partition block @ "
			    "%u, error = %d\n", pdk->dk_name, nextb, error);
			error = ESRCH;
			goto done;
		}
		pbp = (struct partblock *)bp;
		
		if (be32toh(pbp->id) != PARTBLOCK_ID) {
			aprint_error(
			    "%s: RDB partition block @ %u bad id\n",
			    pdk->dk_name, nextb);
			error = ESRCH;
			goto done;
		}
		if (rdbchksum(pbp)) {
			aprint_error(
			    "%s: RDB partition block @ %u bad checksum\n",
			    pdk->dk_name, nextb);
			error = ESRCH;
			goto done;
		}

		tabsize = be32toh(pbp->e.tabsize);
		if (tabsize < 11) {
			/*
			 * not enough info, too funky for us.
			 * I don't want to skip I want it fixed.
			 */
			aprint_error(
			    "%s: RDB partition block @ %u bad partition info "
			    "(environ < 11)\n",
			    pdk->dk_name, nextb);
			error = ESRCH;
			goto done;
		}

		/*
		 * XXXX should be ">" however some vendors don't know
		 * what a table size is so, we hack for them.
		 * the other checks can fail for all I care but this
		 * is a very common value. *sigh*.
		 */
		if (tabsize >= 16)
			archtype = getarchtype(be32toh(pbp->e.dostype));
		else
			archtype = ADT_UNKNOWN;

		switch (archtype) {
		case ADT_NETBSDROOT:
			if (root) {
				aprint_error("%s: more than one root RDB "
				    "partition, ignoring\n", pdk->dk_name);
				continue;
			}
			root = true;
			break;
		case ADT_NETBSDSWAP:
			if (swap) {
				aprint_error("%s: more than one swap RDB "
				    "partition , ignoring\n", pdk->dk_name);
				continue;
			}
			swap = true;
			break;
		default:
			break;
		}

		if (pbp->partname[0] + 1 < sizeof(pbp->partname))
			pbp->partname[pbp->partname[0] + 1] = 0;
		else
			pbp->partname[sizeof(pbp->partname) - 1] = 0;
		strlcpy(dkw.dkw_wname, pbp->partname + 1,
		    sizeof(dkw.dkw_wname));

		ptype = archtype_to_ptype(archtype);
		strlcpy(dkw.dkw_ptype, ptype, sizeof(dkw.dkw_ptype));

		blk_per_cyl =
		    be32toh(pbp->e.secpertrk) * be32toh(pbp->e.numheads)
		    * ((be32toh(pbp->e.sizeblock) << 2) / secsize);
		dkw.dkw_size = (uint64_t)(be32toh(pbp->e.highcyl)
		    - be32toh(pbp->e.lowcyl) + 1) * blk_per_cyl;
		dkw.dkw_offset = (daddr_t)be32toh(pbp->e.lowcyl) * blk_per_cyl;

		error = dkwedge_add(&dkw);
		if (error == EEXIST) {
			aprint_error(
			    "%s: wedge named '%s' already exists, ignoring\n",
			    pdk->dk_name, dkw.dkw_wname);
		} else if (error)
			aprint_error("%s: error %d adding partition %s\n",
			    pdk->dk_name, error, dkw.dkw_wname);
		else
			found = true;
	}

	if (found)
		error = 0;
	else
		error = ESRCH;
done:
	DKW_FREE(bp);
	return error;
}

static unsigned
rdbchksum(void *bdata)
{
	unsigned *blp, cnt, val;

	blp = bdata;
	cnt = be32toh(blp[1]);
	val = 0;

	while (cnt--)
		val += be32toh(*blp++);
	return(val);
}

static unsigned char
getarchtype(unsigned dostype)
{
	unsigned t3, b1;

	t3 = dostype & 0xffffff00;
	b1 = dostype & 0x000000ff;

	switch (t3) {
	case DOST_NBR:
		return ADT_NETBSDROOT;
	case DOST_NBS:
		return ADT_NETBSDSWAP;
	case DOST_NBU:
		return ADT_NETBSDUSER;
	case DOST_MUFS: /* check for 'muFS'? */
	case DOST_DOS:
		return ADT_AMIGADOS;
	case DOST_AMIX:
		return ADT_AMIX;

	case DOST_XXXBSD:
#ifdef DIAGNOSTIC
		aprint_verbose("deprecated dostype found: 0x%x\n",
		    dostype);
#endif
		switch (b1) {
		case 'R':
			return ADT_NETBSDROOT;
		case 'S':
			return ADT_NETBSDSWAP;
		default:
			return ADT_NETBSDUSER;
		}

	case DOST_EXT2:
		return ADT_EXT2;
	case DOST_RAID:
		return ADT_RAID;
	default:
#ifdef DIAGNOSTIC
		aprint_verbose("warning unknown dostype: 0x%x\n",
		    dostype);
#endif
		return ADT_UNKNOWN;
	}
}

static const char *
archtype_to_ptype(unsigned char archtype)
{
	switch (archtype) {
	case ADT_NETBSDROOT:
	case ADT_NETBSDUSER:
		return DKW_PTYPE_FFS;
	case ADT_NETBSDSWAP:
		return DKW_PTYPE_SWAP;
	case ADT_AMIGADOS:
		return DKW_PTYPE_AMIGADOS;
	case ADT_EXT2:
		return DKW_PTYPE_EXT2FS;
	default:
		return DKW_PTYPE_UNKNOWN;
	}
}

#ifdef _KERNEL
DKWEDGE_DISCOVERY_METHOD_DECL(RDB, 15, dkwedge_discover_rdb);
#endif
