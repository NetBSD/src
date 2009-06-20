/* $NetBSD: main.c,v 1.15.4.3 2009/06/20 07:20:08 yamt Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/param.h>
#include <sys/reboot.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <machine/bootinfo.h>

#include "globals.h"

void *bootinfo; /* low memory reserved to pass bootinfo structures */
int bi_size;	/* BOOTINFO_MAXSIZE */
char *bi_next;

extern char bootfile[];	/* filled by DHCP */
char rootdev[4];	/* NIF nickname, filled by netif_init() */
uint8_t en[6];		/* NIC macaddr, fill by netif_init() */

void main(void);
void bi_init(void *);
void bi_add(void *, int, int);

extern char bootprog_rev[], bootprog_maker[], bootprog_date[];

int brdtype;
char *consname = CONSNAME;
int consport = CONSPORT;
int consspeed = CONSSPEED;

void
main(void)
{
	int n, b, d, f, howto;
	unsigned memsize, tag;
	unsigned long marks[MARK_MAX];
	struct btinfo_memory bi_mem;
	struct btinfo_console bi_cons;
	struct btinfo_clock bi_clk;
	struct btinfo_bootpath bi_path;
	struct btinfo_rootdevice bi_rdev;
	unsigned lnif[1][2], lata[1][2];

	/* determine SDRAM size */
	memsize = mpc107memsize();

	printf("\n");
	printf(">> NetBSD/sandpoint Boot, Revision %s\n", bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);
	switch (brdtype) {
	case BRD_SANDPOINTX3:
		printf("Sandpoint X3"); break;
	case BRD_ENCOREPP1:
		printf("Encore PP1"); break;
	}
	printf(", %dMB SDRAM\n", memsize >> 20);

	n = pcilookup(PCI_CLASS_IDE, lata, sizeof(lata)/sizeof(lata[0]));
	if (n == 0)
		printf("no IDE found\n");
	else {
		tag = lata[0][1];
		pcidecomposetag(tag, &b, &d, &f);
		printf("%04x.%04x IDE %02d:%02d:%02d\n",
		    PCI_VENDOR(lata[0][0]), PCI_PRODUCT(lata[0][0]),
		    b, d, f);
	}

	n = pcilookup(PCI_CLASS_ETH, lnif, sizeof(lnif)/sizeof(lnif[0]));
	if (n == 0) {
		tag = ~0;
		printf("no NIC found\n");
	}
	else {
		tag = lnif[0][1];
		pcidecomposetag(tag, &b, &d, &f);
		printf("%04x.%04x NIC %02d:%02d:%02d\n",
		    PCI_VENDOR(lnif[0][0]), PCI_PRODUCT(lnif[0][0]),
		    b, d, f);
	}

	pcisetup();
	pcifixup();

	if (netif_init(tag) == 0)
		printf("no NIC device driver is found\n");

	printf("Try NFS load /netbsd\n");
	marks[MARK_START] = 0;
	if (loadfile("net:", marks, LOAD_KERNEL) < 0) {
		printf("load failed. Restarting...\n");
		_rtt();
	}

	howto = RB_SINGLE | AB_VERBOSE;
#ifdef START_DDB_SESSION
	howto |= RB_KDB;
#endif

	bootinfo = (void *)0x4000;
	bi_init(bootinfo);

	bi_mem.memsize = memsize;
	snprintf(bi_cons.devname, sizeof(bi_cons.devname), consname);
	bi_cons.addr = consport;
	bi_cons.speed = consspeed;
	bi_clk.ticks_per_sec = TICKS_PER_SEC;
	snprintf(bi_path.bootpath, sizeof(bi_path.bootpath), bootfile);
	snprintf(bi_rdev.devname, sizeof(bi_rdev.devname), rootdev);
	bi_rdev.cookie = tag; /* PCI tag for fxp netboot case */

	bi_add(&bi_cons, BTINFO_CONSOLE, sizeof(bi_cons));
	bi_add(&bi_mem, BTINFO_MEMORY, sizeof(bi_mem));
	bi_add(&bi_clk, BTINFO_CLOCK, sizeof(bi_clk));
	bi_add(&bi_path, BTINFO_BOOTPATH, sizeof(bi_path));
	bi_add(&bi_rdev, BTINFO_ROOTDEVICE, sizeof(bi_rdev));

	printf("entry=%p, ssym=%p, esym=%p\n",
	    (void *)marks[MARK_ENTRY],
	    (void *)marks[MARK_SYM],
	    (void *)marks[MARK_END]);

	__syncicache((void *)marks[MARK_ENTRY],
	    (u_int)marks[MARK_SYM] - (u_int)marks[MARK_ENTRY]);

	run((void *)marks[MARK_SYM], (void *)marks[MARK_END],
	    (void *)howto, bootinfo, (void *)marks[MARK_ENTRY]);

	/* should never come here */
	printf("exec returned. Restarting...\n");
	_rtt();
}

void
bi_init(void *addr)
{
	struct btinfo_magic bi_magic;

	memset(addr, 0, BOOTINFO_MAXSIZE);
	bi_next = (char *)addr;
	bi_size = 0;

	bi_magic.magic = BOOTINFO_MAGIC;
	bi_add(&bi_magic, BTINFO_MAGIC, sizeof(bi_magic));
}

void
bi_add(void *new, int type, int size)
{
	struct btinfo_common *bi;

	if (bi_size + size > BOOTINFO_MAXSIZE)
		return;				/* XXX error? */

	bi = new;
	bi->next = size;
	bi->type = type;
	memcpy(bi_next, new, size);
	bi_next += size;
}

#if 0
static const char *cmdln[] = {
	"console=ttyS0,115200 root=/dev/sda1 rw initrd=0x200000,2M",
	"console=ttyS0,115200 root=/dev/nfs ip=dhcp"
};

void
mkatagparams(unsigned addr, char *kcmd)
{
	struct tag {
		unsigned siz;
		unsigned tag;
		unsigned val[1];
	};
	struct tag *p;
#define ATAG_CORE 	0x54410001
#define ATAG_MEM	0x54410002
#define ATAG_INITRD	0x54410005
#define ATAG_CMDLINE	0x54410009
#define ATAG_NONE	0x00000000
#define tagnext(p) (struct tag *)((unsigned *)(p) + (p)->siz)
#define tagsize(n) (2 + (n))

	p = (struct tag *)addr;
	p->tag = ATAG_CORE;
	p->siz = tagsize(3);
	p->val[0] = 0;		/* flags */
	p->val[1] = 0;		/* pagesize */
	p->val[2] = 0;		/* rootdev */
	p = tagnext(p);
	p->tag = ATAG_MEM;
	p->siz = tagsize(2);
	p->val[0] = 64 * 1024 * 1024;
	p->val[1] = 0;		/* start */
	p = tagnext(p);
	if (kcmd != NULL) {
		p = tagnext(p);
		p->tag = ATAG_CMDLINE;
		p->siz = tagsize((strlen(kcmd) + 1 + 3) >> 2);
		strcpy((void *)p->val, kcmd);
	}
	p = tagnext(p);
	p->tag = ATAG_NONE;
	p->siz = 0;
}
#endif
