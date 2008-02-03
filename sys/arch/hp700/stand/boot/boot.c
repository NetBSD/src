/*	$NetBSD: boot.c,v 1.9 2008/02/03 12:09:41 skrll Exp $	*/

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
 *	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

/*
 * Copyright (c) 1998 Michael Shalayeff
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
 *	This product includes software developed by Michael Shalayeff.
 *	This product includes software developed by Tobias Weingartner.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/boot_flag.h>

#include <arch/hp700/stand/common/libsa.h>
#include <lib/libsa/loadfile.h>

#include <machine/pdc.h>
#include <machine/vmparam.h>

#include <arch/hp700/stand/common/dev_hppa.h>

#include "bootinfo.h"

/*
 * Boot program... bits in `howto' determine whether boot stops to
 * ask for system name.	 Boot device is derived from ROM provided
 * information.
 */

#define	MAXLEN	100

char line[MAXLEN];
char devname_buffer[16];

extern	u_int opendev;
extern	char *lowram;
extern	int noconsole;

/*
 * XXX UFS accepts a /, NFS doesn't.
 */
char *name;
char *names[] = {
	"netbsd",		"netbsd.gz",
	"netbsd.bak",		"netbsd.bak.gz",
	"netbsd.old",		"netbsd.old.gz",
	"onetbsd",		"onetbsd.gz",
	NULL
};
#define NUMNAMES	(sizeof(names) / sizeof(char *))

void boot(dev_t boot_dev);
int main(void);
void getbootdev(int *);
void exec_hp700(char *, u_long, int);

int tgets(char *);
void _rtt(void);

typedef void (*startfuncp)(int, int, int, int, int, void *)
    __attribute__ ((noreturn));

int howto;

void
boot(dev_t boot_dev)
{
        machdep();
#ifdef	DEBUGBUG
	debug = 1;
#endif
	devboot(boot_dev, devname_buffer);
	main();
}

int
main(void)
{
	int currname = 0;
	char *filename, filename_buffer[MAXLEN];

	printf("\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);  
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);
	printf(">> Enter \"reset\" to reset system.\n");

	for (;;) {
		size_t size;

		/* reset bootinfo structure */
		bi_init();

		name = names[currname++];
		if (currname == NUMNAMES)
			currname = 0;

		if (!noconsole) {
			howto = 0;
			getbootdev(&howto);
		} else
			printf(": %s\n", name);
		if (strchr(name, ':') != 0) {
			filename = name;
		} else {
			strcpy(filename_buffer, devname_buffer);
			strcat(filename_buffer, ":");
			strcat(filename_buffer, name);
			filename = filename_buffer;
		}

		size = sizeof(struct btinfo_common) + strlen(name) + 1;
	        /* Impose limit (somewhat arbitrary) */
		if (size < BOOTINFO_MAXSIZE / 2) {
			union {
				struct btinfo_kernelfile bi_file;
				char x[size];
			} U;
			strcpy(U.bi_file.name, name);
			BI_ADD(&U.bi_file, BTINFO_KERNELFILE, size);
		}

		exec_hp700(filename, 0, howto);
		printf("boot: %s\n", strerror(errno));
	}
}

void
getbootdev(int *boot_howto)
{
	char c, *ptr = line;
	int bdev, badapt, bctlr, bunit, bpart;

	bdev   = B_TYPE(bootdev);
	badapt = B_ADAPTOR(bootdev);
	bctlr  = B_CONTROLLER(bootdev);
	bunit  = B_UNIT(bootdev);
	bpart  = B_PARTITION(bootdev);

	printf("Boot: [[[%s%d%c:]%s][-a][-c][-d][-s][-v][-q]] :- ",
	    devsw[bdev].dv_name, badapt << 8 | bctlr << 4 | bunit,
	    'a' + bpart, name);

	if (tgets(line)) {
		if (strcmp(line, "reset") == 0) {
			_rtt();
			printf("panic: can't reboot\n");
			return;
		}
		while ((c = *ptr) != '\0') {
			while (c == ' ')
				c = *++ptr;
			if (!c)
				return;
			if (c == '-')
				while ((c = *++ptr) && c != ' ')
					BOOT_FLAG(c, *boot_howto);
			else {
				name = ptr;
				while ((c = *++ptr) && c != ' ');
				if (c)
					*ptr++ = 0;
			}
		}
	} else
		printf("\n");
}

#define	round_to_size(x) \
	(((x) + sizeof(u_long) - 1) & ~(sizeof(u_long) - 1))

void
exec_hp700(char *file, u_long loadaddr, int boot_howto)
{
#ifdef EXEC_DEBUG
	extern int debug;
	int i;
#endif
	struct btinfo_symtab bi_syms;
	u_long marks[MARK_MAX];
	int fd;

	marks[MARK_START] = loadaddr;
#ifdef EXEC_DEBUG
	printf("file=%s loadaddr=%x howto=%x\n",
		file, loadaddr, boot_howto);
#endif
	if ((fd = loadfile(file, marks, LOAD_KERNEL)) == -1)
		return;

	marks[MARK_END] = round_to_size(marks[MARK_END] - loadaddr);
	printf("Start @ 0x%lx [%ld=0x%lx-0x%lx]...\n",
	    marks[MARK_ENTRY], marks[MARK_NSYM],
	    marks[MARK_SYM], marks[MARK_END]);

#ifdef EXEC_DEBUG
	if (debug) {
		printf("ep=0x%x [", marks[MARK_ENTRY]);
		for (i = 0; i < 10240; i++) {
			if (!(i % 8)) {
				printf("\b\n%p:", &((u_int *)marks[MARK_ENTRY])[i]);
				if (getchar() != ' ')
					break;
			}
			printf("%x,", ((int *)marks[MARK_ENTRY])[i]);
		}
		printf("\b\b ]\n");
	}
#endif

	bi_syms.nsym = marks[MARK_NSYM];
	bi_syms.ssym = marks[MARK_SYM];
	bi_syms.esym = marks[MARK_END];
	BI_ADD(&bi_syms, BTINFO_SYMTAB, sizeof(bi_syms));

	fcacheall();

	__asm("mtctl %r0, %cr17");
	__asm("mtctl %r0, %cr17");

	/* stack and the gung is ok at this point, so, no need for asm setup */
	(*(startfuncp)(marks[MARK_ENTRY])) ((int)pdc, boot_howto, bootdev,
	     marks[MARK_END], BOOTARG_APIVER, &bootinfo);
	/* not reached */
}
