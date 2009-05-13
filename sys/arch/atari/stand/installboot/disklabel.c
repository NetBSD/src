/*	$NetBSD: disklabel.c,v 1.3.2.1 2009/05/13 17:16:32 jym Exp $	*/

/*
 * Copyright (c) 1995 Waldi Ravens
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Waldi Ravens.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <sys/disklabel.h>
#include <machine/ahdilabel.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>

#if (BBSIZE < MINBBSIZE)
#error BBSIZE is smaller than MINBBSIZE
#endif

struct ahdilabel {
	u_int		 nsecs;
	daddr_t		 bslst;
	daddr_t		 bslend;
	u_int		 nroots;
	daddr_t		 *roots;
	u_int		 nparts;
	struct ahdi_part *parts;
};

u_int	dkcksum(struct disklabel *);
u_int32_t readdisklabel(char *, struct disklabel *);

static int  bsd_label(int, off_t, struct disklabel *);
static int  ahdi_label(int, u_int32_t *, struct disklabel *);
static int  ahdi_getparts(int, daddr_t, daddr_t, struct ahdilabel *);

u_int
dkcksum (struct disklabel *dl)
{
	u_int16_t sum  = 0,
		  *st  = (u_int16_t *)dl,
		  *end = (u_int16_t *)&dl->d_partitions[dl->d_npartitions];

	while (st < end)
		sum ^= *st++;
	return(sum);
}

u_int32_t
readdisklabel (char *fn, struct disklabel *dl)
{
	int		 fd, e;
	u_int32_t	 bbsec;

	memset(dl, 0, sizeof *dl);

	if ((fd = open(fn, O_RDONLY)) < 0)
		err(EXIT_FAILURE, "%s", fn);

	/* Try NetBSD/Atari format first */
	if ((e = bsd_label(fd, (off_t)0, dl)) < 0)
		err(EXIT_FAILURE, "%s", fn);
	if (!e)
		return(0);

	/* Try unprotected AHDI format last */
	if ((e = ahdi_label(fd, &bbsec, dl)) < 0)
		err(EXIT_FAILURE, "%s", fn);
	if (!e)
		return(bbsec);

	warnx("%s: Unknown disk label format.", fn);
	return(NO_BOOT_BLOCK);
}

static int
bsd_label (int fd, off_t offs, struct disklabel *label)
{
	struct bootblock bb;
	struct disklabel *p;

	if (lseek(fd, offs, SEEK_SET) != offs)
		return(-1);
	if (read(fd, &bb, sizeof(bb)) != sizeof(bb))
		return(-1);

	p = (struct disklabel *)bb.bb_label;
	if (  (offs == 0 && bb.bb_magic != NBDAMAGIC)
	   || (offs != 0 && bb.bb_magic != AHDIMAGIC)
	   || p->d_npartitions > MAXPARTITIONS
	   || p->d_magic2 != DISKMAGIC
	   || p->d_magic  != DISKMAGIC
	   || dkcksum(p)  != 0
	   )	{
		return(1);
	}

	*label = *p;
	return(0);
}

static int
ahdi_label (int fd, u_int32_t *bbsec, struct disklabel *label)
{
	struct ahdilabel al;
	u_int		 i, j;
	int		 e;

	memset(&al, 0, sizeof(al));
	if ((e = ahdi_getparts(fd, AHDI_BBLOCK, AHDI_BBLOCK, &al)))
		return(e);

	/*
	 * Perform sanity checks.
	 */
	if (al.bslst == 0 || al.bslend == 0)
		return(1);
	if (al.nsecs == 0 || al.nparts == 0)
		return(1);
	if (al.nparts > AHDI_MAXPARTS)
		warnx("Too many AHDI partitions (%u).", al.nparts);
	for (i = 0; i < al.nparts; ++i) {
		struct ahdi_part *p1 = &al.parts[i];
		for (j = 0; j < al.nroots; ++j) {
			daddr_t	aux = al.roots[j];
			if (aux >= p1->ap_st && aux <= p1->ap_end)
				return(1);
		}
		for (j = i + 1; j < al.nparts; ++j) {
			struct ahdi_part *p2 = &al.parts[j];
			if (p1->ap_st >= p2->ap_st && p1->ap_st <= p2->ap_end)
				return(1);
			if (p2->ap_st >= p1->ap_st && p2->ap_st <= p1->ap_end)
				return(1);
		}
		if (p1->ap_st >= al.bslst && p1->ap_st <= al.bslend)
			return(1);
		if (al.bslst >= p1->ap_st && al.bslst <= p1->ap_end)
			return(1);
	}

	/*
	 * Search for a NetBSD boot block
	 */
	for (i = 0; i < al.nparts; ++i) {
		struct ahdi_part *pd = &al.parts[i];
		u_int id = *((u_int32_t *)&pd->ap_flg);

		if (id == AHDI_PID_NBD || id == AHDI_PID_RAW) {
			off_t	offs = pd->ap_st * AHDI_BSIZE;
			if ((e = bsd_label(fd, offs, label)) < 0)
				return(e);
			if (!e) {
				*bbsec = pd->ap_st;	/* got it */
				return(0);
			}
		}
	}
	*bbsec = NO_BOOT_BLOCK;	/* AHDI label, no NetBSD boot block */
	return(0);
}

static int
ahdi_getparts(fd, rsec, esec, alab)
	int		 fd;
	daddr_t		 rsec,
			 esec;
	struct ahdilabel *alab;
{
	struct ahdi_part *part, *end;
	struct ahdi_root root;
	off_t		 ro;

	ro = rsec * AHDI_BSIZE;
	if (lseek(fd, ro, SEEK_SET) != ro) {
		off_t	mend = lseek(fd, 0, SEEK_END);
		if (mend == -1 || mend > ro)
			return(-1);
		return(1);
	}
	if (read(fd, &root, sizeof(root)) != sizeof(root))
		return(-1);

	if (rsec == AHDI_BBLOCK)
		end = &root.ar_parts[AHDI_MAXRPD];
	else end = &root.ar_parts[AHDI_MAXARPD];
	for (part = root.ar_parts; part < end; ++part) {
		u_int	id = *((u_int32_t *)&part->ap_flg);
		if (!(id & 0x01000000))
			continue;
		if ((id &= 0x00ffffff) == AHDI_PID_XGM) {
			int	e;
			daddr_t	aux = part->ap_st + esec;
			alab->roots = realloc(alab->roots,
					(alab->nroots + 1) * sizeof(*alab->roots));
			alab->roots[alab->nroots++] = aux;
			e = ahdi_getparts(fd, aux,
					   esec == AHDI_BBLOCK ? aux : esec, alab);
			if (e)
				return(e);
		} else {
			struct ahdi_part *p;
			alab->parts = realloc(alab->parts,
					(alab->nparts + 1) * sizeof(*alab->parts));
			p = &alab->parts[alab->nparts++];
			*((u_int32_t *)&p->ap_flg) = id;
			p->ap_st = part->ap_st + rsec;
			p->ap_end  = p->ap_st + part->ap_size - 1;
		}
	}
	alab->nsecs  = root.ar_hdsize;
	alab->bslst  = root.ar_bslst;
	alab->bslend = root.ar_bslst + root.ar_bslsize - 1;
	return(0);
}
