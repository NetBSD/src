/*	$NetBSD: boot.c,v 1.2.6.3 2000/12/08 09:28:48 bouyer Exp $	*/

/*-
 * Copyright (C) 1999 Izumi Tsutsui.  All rights reserved.
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

#include <machine/romcall.h>

void ICIA __P((void));
extern char edata[], end[];

char *devs[] = { "hd", "fh", "fd", NULL, NULL, "rd", "st" };
char *kernels[] = { "/netbsd", "/netbsd.gz", NULL };

#ifdef BOOT_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

void
boot(d4, d5, d6, d7)
	int d4, d5, d6, d7;
{
	int fd, i;
	int ctlr, unit, part, type;
	int bootdev = d6;
	char *netbsd = (char *)d5;
	u_long marks[MARK_MAX];
	char devname[32], file[32];
	void (*entry)();

	printf("\n");
	printf("NetBSD/news68k Secondary Boot\n");

	/* bootname is "boot" by default. */
	if (netbsd == NULL || strcmp(netbsd, "boot") == 0 ||
			      strcmp(netbsd, "/boot") == 0)
		netbsd = "";

	DPRINTF("howto = 0x%x\n", d7);
	DPRINTF("bootdev = 0x%x\n", bootdev);
	DPRINTF("bootname = %s\n", netbsd);
	DPRINTF("maxmem = 0x%x\n", d4);

#define	SET_MAGIC(bootdev, magic) ((bootdev & 0x0fffffff)| (magic << 28))
	/* PROM monitor passes 0xa, but NEWS-OS /boot passed 0x5... */
	bootdev = SET_MAGIC(bootdev, 5);

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
		fd = loadfile(file, marks, LOAD_KERNEL);
		if (fd != -1)
			break;
	}
	if (fd == -1) {
#if 0 /* bootxx() may be overrided by loaded kernel */
		return;
#else
		rom_halt();
		/* NOTREACHED */
#endif
	}

	DPRINTF("entry = 0x%x\n", (int)marks[MARK_ENTRY]);
	DPRINTF("ssym = 0x%x\n", (int)marks[MARK_SYM]);
	DPRINTF("esym = 0x%x\n", (int)marks[MARK_END]);

	entry = (void *)marks[MARK_ENTRY];

	printf("\n");

	ICIA();
	__asm __volatile ("movl %0,%%d7" : : "m" (d7));
	__asm __volatile ("movl %0,%%d6" : : "m" (bootdev));
	__asm __volatile ("movl %0,%%d5" : : "m" (netbsd));
	__asm __volatile ("movl %0,%%d4" : : "m" (d4));
	__asm __volatile ("movl %0,%%d2" : : "m" (marks[MARK_END]));
	(*((int (*)())entry))();
}
