/*	$NetBSD: boot.c,v 1.32.6.2 2016/07/09 20:24:58 skrll Exp $ */
/*-
 * Copyright (c) 1982, 1986 The Regents of the University of California.
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
 *	@(#)boot.c	7.15 (Berkeley) 5/4/91
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/boot_flag.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#define V750UCODE(x)    ((x>>8)&255)

#include "machine/rpb.h"

#include "vaxstand.h"

/*
 * Boot program... arguments passed in r10 and r11 determine
 * whether boot stops to ask for system name and which device
 * boot comes from.
 */

char line[100];
int	bootdev, debug;
extern	unsigned opendev;

void	usage(char *), boot(char *), halt(char *);
void	Xmain(void);
void	autoconf(void);
int	setjmp(int *);
int	testkey(void);
void	loadpcs(void);

const struct vals {
	char	*namn;
	void	(*func)(char *);
	char	*info;
} val[] = {
	{"?", usage, "Show this help menu"},
	{"help", usage, "Same as '?'"},
	{"boot", boot, "Load and execute file"},
	{"halt", halt, "Halts the system"},
	{0, 0},
};

static struct {
	char name[12];
	int quiet;
} filelist[] = {
	{ "netbsd.vax", 1 },
	{ "netbsd", 0 },
	{ "netbsd.gz", 0 },
	{ "netbsd.old", 0 },
	{ "gennetbsd", 0 },
	{ "", 0 },
};

int jbuf[10];
int sluttid, senast, skip, askname;
struct rpb bootrpb;

void
Xmain(void)
{
	int j, nu, fd;
	u_long marks[MARK_MAX];
	extern const char bootprog_rev[];

	skip = 1;
	autoconf();

	askname = bootrpb.rpb_bootr5 & RB_ASKNAME;
	printf("\n\r>> NetBSD/vax boot [%s] <<\n", bootprog_rev);
	printf(">> Press any key to abort autoboot  ");
	sluttid = getsecs() + 5;
	senast = 0;
	skip = 0;
	setjmp(jbuf);
	for (;;) {
		nu = sluttid - getsecs();
		if (senast != nu)
			printf("%c%d", 8, nu);
		if (nu <= 0)
			break;
		senast = nu;
		if ((j = (testkey() & 0177))) {
			skip = 1;
			if (j != 10 && j != 13) {
				printf("\nPress '?' for help");
				askname = 1;
			}
			break;
		}
	}
	skip = 1;
	printf("\n");
	if (setjmp(jbuf))
		askname = 1;

	/* First try to autoboot */
	if (askname == 0) {
		int fileindex;
		for (fileindex = 0; filelist[fileindex].name[0] != '\0';
		    fileindex++) {
			errno = 0;
			if (!filelist[fileindex].quiet)
				printf("> boot %s\n", filelist[fileindex].name);
			marks[MARK_START] = 0;
			fd = loadfile(filelist[fileindex].name, marks,
			    LOAD_KERNEL|COUNT_KERNEL);
			if (fd >= 0) {
				close(fd);
				machdep_start((char *)marks[MARK_ENTRY],
						      marks[MARK_NSYM],
					      (void *)marks[MARK_START],
					      (void *)marks[MARK_SYM],
					      (void *)marks[MARK_END]);
			}
			if (!filelist[fileindex].quiet)
				printf("%s: boot failed: %s\n",
				    filelist[fileindex].name, strerror(errno));
#if 0 /* Will hang VAX 4000 machines */
			if (testkey())
				break;
#endif
		}
	}

	/* If any key pressed, go to conversational boot */
	for (;;) {
		const struct vals *v = &val[0];
		char *c, *d;

		printf("> ");
		kgets(line, sizeof(line));

		c = line;
		while (*c == ' ')
			c++;

		if (c[0] == 0)
			continue;

		if ((d = strchr(c, ' ')))
			*d++ = 0;

		while (v->namn) {
			if (strcmp(v->namn, c) == 0)
				break;
			v++;
		}
		if (v->namn)
			(*v->func)(d);
		else
			printf("Unknown command: %s\n", c);
	}
}

void
halt(char *hej)
{
	__asm("halt");
}

void
boot(char *arg)
{
	char *fn = "netbsd";
	int howto, fl, fd;
	u_long marks[MARK_MAX];

	if (arg) {
		while (*arg == ' ')
			arg++;

		if (*arg != '-') {
			fn = arg;
			if ((arg = strchr(arg, ' '))) {
				*arg++ = 0;
				while (*arg == ' ')
					arg++;
			} else
				goto load;
		}
		if (*arg != '-') {
fail:			printf("usage: boot [filename] [-asdqv]\n");
			return;
		}

		howto = 0;
		while (*++arg) {
			fl = 0;
			BOOT_FLAG(*arg, fl);
			if (!fl)
				goto fail;
			howto |= fl;
		}
		bootrpb.rpb_bootr5 = howto;
	}
load:
	marks[MARK_START] = 0;
	fd = loadfile(fn, marks, LOAD_KERNEL|COUNT_KERNEL);
	if (fd >= 0) {
		close(fd);
		machdep_start((char *)marks[MARK_ENTRY],
				      marks[MARK_NSYM],
			      (void *)marks[MARK_START],
			      (void *)marks[MARK_SYM],
			      (void *)marks[MARK_END]);
	}
	printf("Boot failed: %s\n", strerror(errno));
}

/* 750 Patchable Control Store magic */

#include "../include/mtpr.h"
#include "../include/cpu.h"
#include "../include/sid.h"
#define	PCS_BITCNT	0x2000		/* number of patchbits */
#define	PCS_MICRONUM	0x400		/* number of ucode locs */
#define	PCS_PATCHADDR	0xf00000	/* start addr of patchbits */
#define	PCS_PCSADDR	(PCS_PATCHADDR+0x8000)	/* start addr of pcs */
#define	PCS_PATCHBIT	(PCS_PATCHADDR+0xc000)	/* patchbits enable reg */
#define	PCS_ENABLE	0xfff00000	/* enable bits for pcs */

#define	extzv(one, two, three,four)	\
({			\
	__asm volatile ("extzv %0,%3,%1,%2"	\
			:			\
			: "g"(one),"m"(two),"mo>"(three),"g"(four));	\
})


void
loadpcs(void)
{
	static int pcsdone = 0;
	int mid = mfpr(PR_SID);
	int i, j, *ip, *jp;
	char pcs[100];
	char *cp;

	if ((mid >> 24) != VAX_750 || ((mid >> 8) & 255) < 95 || pcsdone)
		return;
	printf("Updating 11/750 microcode: ");
	for (cp = line; *cp; cp++)
		if (*cp == ')' || *cp == ':')
			break;
	if (*cp) {
		memcpy(pcs, line, 99);
		pcs[99] = 0;
		i = cp - line + 1;
	} else
		i = 0;
	strcpy(pcs + i, "pcs750.bin");
	i = open(pcs, 0);
	if (i < 0) {
		printf("bad luck - missing pcs750.bin :-(\n");
		return;
	}
	/*
	 * We ask for more than we need to be sure we get only what we expect.
	 * After read:
	 *	locs 0 - 1023	packed patchbits
	 *	 1024 - 11264	packed microcode
	 */
	if (read(i, (char *)0, 23*512) != 22*512) {
		printf("Error reading %s\n", pcs);
		close(i);
		return;
	}
	close(i);

	/*
	 * Enable patchbit loading and load the bits one at a time.
	 */
	*((int *)PCS_PATCHBIT) = 1;
	ip = (int *)PCS_PATCHADDR;
	jp = (int *)0;
	for (i=0; i < PCS_BITCNT; i++) {
		extzv(i,*jp,*ip++,1);
	}
	*((int *)PCS_PATCHBIT) = 0;

	/*
	 * Load PCS microcode 20 bits at a time.
	 */
	ip = (int *)PCS_PCSADDR;
	jp = (int *)1024;
	for (i=j=0; j < PCS_MICRONUM * 4; i+=20, j++) {
		extzv(i,*jp,*ip++,20);
	}

	/*
	 * Enable PCS.
	 */
	i = *jp;		/* get 1st 20 bits of microcode again */
	i &= 0xfffff;
	i |= PCS_ENABLE;	/* reload these bits with PCS enable set */
	*((int *)PCS_PCSADDR) = i;

	mid = mfpr(PR_SID);
	printf("new rev level=%d\n", V750UCODE(mid));
	pcsdone = 1;
}

void
usage(char *hej)
{
	const struct vals *v = &val[0];

	printf("Commands:\n");
	while (v->namn) {
		printf("%s\t%s\n", v->namn, v->info);
		v++;
	}
}
