/* $NetBSD: mt.c,v 1.49 2022/01/24 09:14:36 andvar Exp $ */

/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mt.c	8.2 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: mt.c,v 1.49 2022/01/24 09:14:36 andvar Exp $");
#endif
#endif /* not lint */

/*
 * mt --
 *   magnetic tape manipulation program
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <paths.h>
#include <rmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* pseudo ioctl constants */
#define MTASF	100

struct commands {
	const char *c_name;		/* command */
	size_t c_namelen;		/* command len */
	u_long c_spcl;			/* ioctl request */
	int c_code;			/* ioctl code for MTIOCTOP command */
	int c_ronly;			/* open tape read-only */
	int c_mincount;			/* min allowed count value */
};

#define CMD(a)	a, sizeof(a) - 1
static const struct commands com[] = {
	{ CMD("asf"),		MTIOCTOP,     MTASF,      1,  0 },
	{ CMD("blocksize"),	MTIOCTOP,     MTSETBSIZ,  1,  0 },
	{ CMD("bsf"),		MTIOCTOP,     MTBSF,      1,  1 },
	{ CMD("bsr"),		MTIOCTOP,     MTBSR,      1,  1 },
	{ CMD("compress"),	MTIOCTOP,     MTCMPRESS,  1,  0 },
	{ CMD("density"),	MTIOCTOP,     MTSETDNSTY, 1,  0 },
	{ CMD("eof"),		MTIOCTOP,     MTWEOF,     0,  1 },
	{ CMD("eom"),		MTIOCTOP,     MTEOM,      1,  0 },
	{ CMD("erase"),		MTIOCTOP,     MTERASE,    0,  0 },
	{ CMD("fsf"),		MTIOCTOP,     MTFSF,      1,  1 },
	{ CMD("fsr"),		MTIOCTOP,     MTFSR,      1,  1 },
	{ CMD("offline"),	MTIOCTOP,     MTOFFL,     1,  0 },
	{ CMD("rdhpos"),	MTIOCRDHPOS,  0,          1,  0 },
	{ CMD("rdspos"),	MTIOCRDSPOS,  0,          1,  0 },
	{ CMD("retension"),	MTIOCTOP,     MTRETEN,    1,  0 },
	{ CMD("rewind"),	MTIOCTOP,     MTREW,      1,  0 },
	{ CMD("rewoffl"),	MTIOCTOP,     MTOFFL,     1,  0 },
	{ CMD("setblk"),	MTIOCTOP,     MTSETBSIZ,  1,  0 },
	{ CMD("setdensity"),	MTIOCTOP,     MTSETDNSTY, 1,  0 },
	{ CMD("sethpos"),	MTIOCHLOCATE, 0,          1,  0 },
	{ CMD("setspos"),	MTIOCSLOCATE, 0,          1,  0 },
	{ CMD("status"),	MTIOCGET,     MTNOP,      1,  0 },
	{ CMD("weof"),		MTIOCTOP,     MTWEOF,     0,  1 },
	{ CMD("eew"),		MTIOCTOP,     MTEWARN,    1,  0 },
	{ CMD("cache"),		MTIOCTOP,     MTCACHE,    1,  0 },
	{ CMD("nocache"),	MTIOCTOP,     MTNOCACHE,  1,  0 },
	{ .c_name = NULL }
};

static void printreg(const char *, u_int, const char *);
static void status(struct mtget *);
__dead static void usage(void);

int
main(int argc, char *argv[])
{
	const struct commands *cp, *comp;
	struct mtget mt_status;
	struct mtop mt_com;
	int ch, mtfd, flags;
	char *p;
	const char *tape;
	int count;
	size_t len;

	setprogname(argv[0]);
	if ((tape = getenv("TAPE")) == NULL)
		tape = _PATH_DEFTAPE;

	while ((ch = getopt(argc, argv, "f:t:")) != -1)
		switch (ch) {
		case 'f':
		case 't':
			tape = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc < 1 || argc > 2)
		usage();

	len = strlen(p = *argv++);
	for (comp = NULL, cp = com; cp->c_name != NULL; cp++) {
		size_t clen = MIN(len, cp->c_namelen);
		if (strncmp(p, cp->c_name, clen) == 0) {
			if (comp != NULL)
				errx(1, "%s: Ambiguous command `%s' or `%s'?",
				    p, cp->c_name, comp->c_name);
			else
				comp = cp;
		}
	}
	if (comp == NULL)
		errx(1, "%s: unknown command", p);

	if (*argv) {
		count = strtol(*argv, &p, 10);
		if (count < comp->c_mincount || *p)
			errx(2, "%s: illegal count", *argv);
	} else
		count = 1;

	flags = comp->c_ronly ? O_RDONLY : O_WRONLY;

	if ((mtfd = open(tape, flags)) < 0)
		err(2, "%s", tape);

	switch (comp->c_spcl) {
	case MTIOCTOP:
		if (comp->c_code == MTASF) {

			/* If mtget.mt_fileno was implemented, We could
			   compute the minimal seek needed to position
			   the tape.  Until then, rewind and seek from
			   beginning-of-tape */

			mt_com.mt_op = MTREW;
			mt_com.mt_count = 1;
			if (ioctl(mtfd, MTIOCTOP, &mt_com) < 0)
				err(2, "%s", tape);
		
			if (count > 0) {
				mt_com.mt_op = MTFSF;
				    mt_com.mt_count = count;
				if (ioctl(mtfd, MTIOCTOP, &mt_com) < 0)
				    err(2, "%s", tape);
			}

		} else {
			mt_com.mt_op = comp->c_code;
			mt_com.mt_count = count;

			if (ioctl(mtfd, MTIOCTOP, &mt_com) < 0)
				err(2, "%s: %s", tape, comp->c_name);

		}
		break;

	case MTIOCGET:
		if (ioctl(mtfd, MTIOCGET, &mt_status) < 0)
			err(2, "%s: %s", tape, comp->c_name);
		status(&mt_status);
		break;

	case MTIOCRDSPOS:
	case MTIOCRDHPOS:
		if (ioctl(mtfd, comp->c_spcl, (caddr_t) &count) < 0)
			err(2, "%s", tape);
		printf("%s: block location %u\n", tape, (unsigned int) count);
		break;

	case MTIOCSLOCATE:
	case MTIOCHLOCATE:
		if (ioctl(mtfd, comp->c_spcl, (caddr_t) &count) < 0)
			err(2, "%s", tape);
		break;

	default:
		errx(1, "internal error: unknown request %ld", comp->c_spcl);
	}

	exit(0);
	/* NOTREACHED */
}

#if defined(sun) && !defined(__SVR4)
#include <sundev/tmreg.h>
#include <sundev/arreg.h>
#endif

#ifdef tahoe
#include <tahoe/vba/cyreg.h>
#endif

static const struct tape_desc {
	short	t_type;		/* type of magtape device */
	const	char *t_name;	/* printing name */
	const	char *t_dsbits;	/* "drive status" register */
	const	char *t_erbits;	/* "error" register */
} tapes[] = {
#if defined(sun) && !defined(__SVR4)
	{ MT_ISCPC,	"TapeMaster",	TMS_BITS,	0 },
	{ MT_ISAR,	"Archive",	ARCH_CTRL_BITS,	ARCH_BITS },
#endif
#ifdef tahoe
	{ MT_ISCY,	"cipher",	CYS_BITS,	CYCW_BITS },
#endif
#define SCSI_DS_BITS	"\20\5WriteProtect\2Mounted"
	{ 0x7,		"SCSI",		SCSI_DS_BITS,	"76543210" },
	{ .t_type = 0 }
};


/*
 * Interpret the status buffer returned
 */
static void
status(struct mtget *bp)
{
	const struct tape_desc *mt;

	for (mt = tapes;; mt++) {
		if (mt->t_type == 0) {
			(void)printf("%d: unknown tape drive type\n",
			    bp->mt_type);
			return;
		}
		if (mt->t_type == bp->mt_type)
			break;
	}
	(void)printf("%s tape drive, residual=%d\n", mt->t_name, bp->mt_resid);
	printreg("ds", bp->mt_dsreg, mt->t_dsbits);
	printreg("\ner", bp->mt_erreg, mt->t_erbits);
	(void)putchar('\n');
	(void)printf("blocksize: %d (%d, %d, %d, %d)\n",
		bp->mt_blksiz, bp->mt_mblksiz[0], bp->mt_mblksiz[1],
		bp->mt_mblksiz[2], bp->mt_mblksiz[3]);
	(void)printf("density: %d (%d, %d, %d, %d)\n",
		bp->mt_density, bp->mt_mdensity[0], bp->mt_mdensity[1],
		bp->mt_mdensity[2], bp->mt_mdensity[3]);
	(void)printf("current file number: %d\n", bp->mt_fileno);
	(void)printf("current block number: %d\n", bp->mt_blkno);
}

/*
 * Print a register a la the %b format of the kernel's printf.
 */
static void
printreg(const char *s, u_int v, const char *bits)
{
	int any, i;
	char c;

	any = 0;
	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	if (v && bits && *++bits) {
		putchar('<');
		while ((i = *bits++)) {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: %s [-f device] command [count]\n",
	    getprogname());
	exit(1);
	/* NOTREACHED */
}
