/*	$NetBSD: boot.c,v 1.10 2001/06/21 03:13:05 uwe Exp $ */

/*-
 * Copyright (c) 1982, 1986, 1990, 1993
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
 * 	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/exec.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include <machine/promlib.h>
#include <sparc/stand/common/promdev.h>

#include "bootinfo.h"

extern void	prom_patch __P((void));	/* prompatch.c */

static int	bootoptions __P((char *));
#if 0
static void	promsyms __P((int, struct exec *));
#endif

int debug;
int netif_debug;

extern char bootprog_name[], bootprog_rev[], bootprog_date[], bootprog_maker[];
unsigned long		esym;
char			*strtab;
int			strtablen;
char			fbuf[80], dbuf[128];

int	main __P((void));
typedef void (*entry_t)__P((caddr_t, int, int, int, long, long));

/*
 * Boot device is derived from ROM provided information, or if there is none,
 * this list is used in sequence, to find a kernel.
 */
char *kernels[] = {
	"netbsd",
	"netbsd.gz",
	"netbsd.old",
	"netbsd.old.gz",
	"onetbsd",
	"onetbsd.gz",
	"vmunix",
#ifdef notyet
	"netbsd.pl",
	"netbsd.pl.gz",
	"netbsd.el",
	"netbsd.el.gz",
#endif
	NULL
};

int
bootoptions(ap)
	char *ap;
{
	int v = 0;
	if (ap == NULL || *ap++ != '-')
		return (0);

	while (*ap != '\0' && *ap != ' ' && *ap != '\t' && *ap != '\n') {
		switch (*ap) {
		case 'a':
			v |= RB_ASKNAME;
			break;
		case 's':
			v |= RB_SINGLE;
			break;
		case 'd':
			v |= RB_KDB;
			debug = 1;
			break;
		}
		ap++;
	}

	return (v);
}

int
main()
{
	int	io, i;
	char	*kernel;
	int	how;
	u_long	marks[MARK_MAX], bootinfo;
	struct btinfo_symtab bi_sym;
	void	*arg;

#ifdef HEAP_VARIABLE
	{
		extern char end[];
		setheap((void *)ALIGN(end), (void *)0xffffffff);
	}
#endif
	prom_init();

	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

	/* massage machine prom */
	prom_patch();

	/*
	 * get default kernel.
	 */
	prom_bootdevice = prom_getbootpath();
	kernel = prom_getbootfile();
	how = bootoptions(prom_getbootargs());

	if (kernel != NULL && *kernel != '\0') {
		i = -1;	/* not using the kernels */
	} else {
		i = 0;
		kernel = kernels[i];
	}

	for (;;) {
		/*
		 * ask for a kernel first ..
		 */
		if (how & RB_ASKNAME) {
			printf("device[%s] (\"halt\" to halt): ",
					prom_bootdevice);
			gets(dbuf);
			if (strcmp(dbuf, "halt") == 0)
				_rtt();
			if (dbuf[0])
				prom_bootdevice = dbuf;
			printf("boot (press RETURN to try default list): ");
			gets(fbuf);
			if (fbuf[0])
				kernel = fbuf;
			else {
				how &= ~RB_ASKNAME;
				i = 0;
				kernel = kernels[i];
			}
		}

		marks[MARK_START] = 0;
		printf("Booting %s\n", kernel);
		if ((io = loadfile(kernel, marks, LOAD_KERNEL)) != -1)
			break;
			
		/*
		 * if we have are not in askname mode, and we aren't using the
		 * prom bootfile, try the next one (if it exits).  otherwise,
		 * go into askname mode.
		 */
		if ((how & RB_ASKNAME) == 0 &&
		    i != -1 && kernels[++i]) {
			kernel = kernels[i];
			printf(": trying %s...\n", kernel);
		} else {
			printf("\n");
			how |= RB_ASKNAME;
		}
	}

	marks[MARK_END] = (((u_long)marks[MARK_END] + sizeof(int) - 1)) &
	    (-sizeof(int));
	arg = (prom_version() == PROM_OLDMON) ? (caddr_t)PROM_LOADADDR : romp;
#if 0
	/* Old style cruft; works only with a.out */
	marks[MARK_END] |= 0xf0000000;
	(*(entry_t)marks[MARK_ENTRY])(arg, 0, 0, 0, marks[MARK_END],
	    DDB_MAGIC1);
#else
	/* Should work with both a.out and ELF, but somehow ELF is busted */
	bootinfo = bi_init(marks[MARK_END]);
	bi_sym.nsym = marks[MARK_NSYM];
	bi_sym.ssym = marks[MARK_SYM];
	bi_sym.esym = marks[MARK_END];
	bi_add(&bi_sym, BTINFO_SYMTAB, sizeof(bi_sym));
	(*(entry_t)marks[MARK_ENTRY])(arg, 0, 0, 0, bootinfo, DDB_MAGIC2);
#endif

	_rtt();
}
