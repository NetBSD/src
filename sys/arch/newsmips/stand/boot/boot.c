/*	$NetBSD: boot.c,v 1.1.8.1 1999/12/27 18:33:10 wrstuden Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include <machine/bootinfo.h>
#include <machine/romcall.h>

void flushicache __P((void *, int));
extern char _edata[], _end[];

char *devs[] = { "sd", "fh", "fd", NULL, NULL, "rd", "st" };
char *kernels[] = { "/netbsd", "/netbsd.gz", NULL };

#ifdef BOOT_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

void
boot(a0, a1, a2, a3, a4, a5)
	int a0, a1, a2, a3, a4, a5;
{
	int fd, i;
	int ctlr, unit, part, type;
	int bootdev = a1;
	char *netbsd = (char *)a2;
	u_long marks[MARK_MAX];
	char devname[32], file[32];
	void (*entry)();
	struct btinfo_symtab bi_sym;

	/* Clear BSS. */
	bzero(_edata, _end - _edata);

	printf("\n");
	printf("NetBSD/newsmips Secondary Boot\n");

	bi_init(BOOTINFO_ADDR);

	/* bootname is "/boot" by default. */
	if (netbsd == NULL || strcmp(netbsd, "/boot") == 0)
		netbsd = "";

	DPRINTF("howto = 0x%x\n", a0);
	DPRINTF("bootdev = 0x%x\n", bootdev);
	DPRINTF("bootname = %s\n", netbsd);
	DPRINTF("maxmem = 0x%x\n", a3);

	ctlr = BOOTDEV_CTLR(bootdev);
	unit = BOOTDEV_UNIT(bootdev);
	part = BOOTDEV_PART(bootdev);
	type = BOOTDEV_TYPE(bootdev);

	marks[MARK_START] = 0;

	if (devs[type] == NULL) {
		printf("unknown bootdev (0x%x)\n", bootdev);
		return;
	}

	sprintf(devname, "%s(%d,%d,%d)", devs[type], ctlr, unit, part);
	printf("Booting %s%s\n", devname, netbsd);

	/* use user specified kernel name if exists */
	if (*netbsd) {
		kernels[0] = netbsd;
		kernels[1] = NULL;
	}

	for (i = 0; kernels[i]; i++) {
		sprintf(file, "%s%s", devname, kernels[i]);
		DPRINTF("trying %s...\n", file);
		fd = loadfile(file, marks, LOAD_ALL);
		if (fd != -1)
			break;
	}
	if (fd == -1)
		return;

	DPRINTF("entry = 0x%x\n", (int)marks[MARK_ENTRY]);
	DPRINTF("ssym = 0x%x\n", (int)marks[MARK_SYM]);
	DPRINTF("esym = 0x%x\n", (int)marks[MARK_END]);

	bi_sym.nsym = marks[MARK_NSYM];
	bi_sym.ssym = marks[MARK_SYM];
	bi_sym.esym = marks[MARK_END];
	bi_add(&bi_sym, BTINFO_SYMTAB, sizeof(bi_sym));

	entry = (void *)marks[MARK_ENTRY];
	flushicache(entry, marks[MARK_SYM] - marks[MARK_ENTRY]);

	printf("\n");
	(*entry)(a0, a1, a2, a3, a4, a5);
}

void
putchar(x)
	int x;
{
	char c = x;

	rom_write(1, &c, 1);
}
