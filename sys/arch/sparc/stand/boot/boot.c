/*	$NetBSD: boot.c,v 1.4 1999/02/15 18:59:36 pk Exp $ */

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
#include <a.out.h>

#include <lib/libsa/stand.h>

#include <machine/promlib.h>
#include <sparc/stand/common/promdev.h>

static int	bootoptions __P((char *));
void		loadfile __P((int, caddr_t));
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

	prom_init();

	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

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

		if ((io = open(kernel, 0)) >= 0)
			break;
		printf("open: %s: %s", kernel, strerror(errno));

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

	/*
	 * XXX
	 * make loadfile() return a value, so that if the load of the kernel
	 * fails, we can jump back and try another kernel in the list.
	 */
	printf("Booting %s @ %p\n", kernel, PROM_LOADADDR);
	loadfile(io, PROM_LOADADDR);

	_rtt();
}

void
loadfile(io, addr)
	int	io;
	caddr_t addr;
{
	entry_t entry = (entry_t)PROM_LOADADDR;
	void *arg;
	struct exec x;
	int i;

	i = read(io, (char *)&x, sizeof(x));
	if (i != sizeof(x) ||
	    N_BADMAG(x)) {
		printf("Bad format\n");
		return;
	}
	printf("%ld", x.a_text);
	if (N_GETMAGIC(x) == ZMAGIC) {
		entry = (entry_t)(addr+sizeof(struct exec));
		addr += sizeof(struct exec);
	}
	if (read(io, (char *)addr, x.a_text) != x.a_text)
		goto shread;
	addr += x.a_text;
	if (N_GETMAGIC(x) == ZMAGIC || N_GETMAGIC(x) == NMAGIC)
		while ((int)addr & __LDPGSZ)
			*addr++ = 0;
	printf("+%ld", x.a_data);
	if (read(io, addr, x.a_data) != x.a_data)
		goto shread;
	addr += x.a_data;
	printf("+%ld", x.a_bss);
	for (i = 0; i < x.a_bss; i++)
		*addr++ = 0;
	if (x.a_syms != 0) {
		bcopy(&x.a_syms, addr, sizeof(x.a_syms));
		addr += sizeof(x.a_syms);
		printf("+[%ld", x.a_syms);
		if (read(io, addr, x.a_syms) != x.a_syms)
			goto shread;
		addr += x.a_syms;

		if (read(io, &strtablen, sizeof(int)) != sizeof(int))
			goto shread;

		bcopy(&strtablen, addr, sizeof(int));
		if ((i = strtablen) != 0) {
			i -= sizeof(int);
			addr += sizeof(int);
			if (read(io, addr, i) != i)
			    goto shread;
			addr += i;
		}
		printf("+%d]", i);
		esym = ((u_int)x.a_entry - (u_int)PROM_LOADADDR) +
			(((int)addr + sizeof(int) - 1) & ~(sizeof(int) - 1));
#if 0
		/*
		 * The FORTH word `loadsyms' is mentioned in the
		 * "Openboot command reference" book, but it seems it has
		 * not been implemented on at least one machine..
		 */
		promsyms(io, &x);
#endif
	}
	printf("=%p\n", addr);
	close(io);

	/* Note: args 2-4 not used due to conflicts with SunOS loaders */
	arg = (prom_version() == PROM_OLDMON) ? PROM_LOADADDR : romp;
	(*entry)(arg, 0, 0, 0, esym, DDB_MAGIC1);
	return;

shread:
	printf("boot: short read\n");
	return;
}

#if 0
struct syms {
	u_int32_t	value;
	u_int32_t	index;
};

static void
sort(syms, n)
	struct syms *syms;
	int n;
{
	register struct syms *sj;
	register int i, j, k;
	register u_int32_t value, index;

	/* Insertion sort.  This is O(n^2), but so what? */
	for (i = 1; i < n; i++) {
		/* save i'th entry */
		value = syms[i].value;
		index = syms[i].index;
		/* find j such that i'th entry goes before j'th */
		for (j = 0, sj = syms; j < i; j++, sj++)
			if (value < sj->value)
				break;
		/* slide up any additional entries */
		for (k = 0; k < (i - j); k++) {
			sj[k+1].value = sj[k].value;
			sj[k+1].index = sj[k].index;
		}
		sj->value = value;
		sj->index = index;
	}
}

void
promsyms(fd, hp)
	int fd;
	struct exec *hp;
{
	int i, n, strtablen;
	char *str, *p, *cp, buf[128];
	struct syms *syms;

	lseek(fd, sizeof(*hp)+hp->a_text+hp->a_data, SEEK_SET);
	n = hp->a_syms/sizeof(struct nlist);
	if (n == 0)
		return;
	syms = (struct syms *)alloc(n * sizeof(struct syms));

	printf("+[%x+", hp->a_syms);
	for (i = 0; i < n; i++) {
		struct nlist nlist;

		if (read(fd, &nlist, sizeof(nlist)) != sizeof(nlist)) {
			printf("promsyms: read failed\n");
			return;
		}
		syms[i].value = nlist.n_value;
		syms[i].index = nlist.n_un.n_strx - sizeof(strtablen);
	}

	sort(syms, n);

	if (read(fd, &strtablen, sizeof(strtablen)) != sizeof(strtablen)) {
		printf("promsym: read failed (strtablen)\n");
		return;
	}
	if (strtablen < sizeof(strtablen)) {
		printf("promsym: string table corrupted\n");
		return;
	}
	strtablen -= sizeof(strtablen);
	str = (char *)alloc(strtablen);

	printf("%x]", strtablen);
	if (read(fd, str, strtablen) != strtablen) {
		printf("promsym: read failed (strtab)\n");
		return;
	}

	sprintf(buf, "%x %d %x loadsyms", syms, n, str);
	prom_interpret(buf);
}
#endif
