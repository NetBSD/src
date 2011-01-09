/* $NetBSD: main.c,v 1.36 2011/01/09 22:59:40 phx Exp $ */

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

static const struct bootarg {
	const char *name;
	int value;
} bootargs[] = {
	{ "multi",	RB_AUTOBOOT },
	{ "auto",	RB_AUTOBOOT },
	{ "ask",	RB_ASKNAME },
	{ "single",	RB_SINGLE },
	{ "ddb",	RB_KDB },
	{ "userconf",	RB_USERCONF },
	{ "norm",	AB_NORMAL },
	{ "quiet",	AB_QUIET },
	{ "verb",	AB_VERBOSE },
	{ "silent",	AB_SILENT },
	{ "debug",	AB_DEBUG }
};

void *bootinfo; /* low memory reserved to pass bootinfo structures */
int bi_size;	/* BOOTINFO_MAXSIZE */
char *bi_next;

void bi_init(void *);
void bi_add(void *, int, int);

struct btinfo_memory bi_mem;
struct btinfo_console bi_cons;
struct btinfo_clock bi_clk;
struct btinfo_prodfamily bi_fam;
struct btinfo_bootpath bi_path;
struct btinfo_rootdevice bi_rdev;
struct btinfo_net bi_net;

void main(int, char **);
extern char bootprog_rev[], bootprog_maker[], bootprog_date[];

int brdtype;
uint32_t busclock, cpuclock;

static int check_bootname(char *);
#define	BNAME_DEFAULT "nfs:"

void
main(int argc, char *argv[])
{
	struct brdprop *brdprop;
	unsigned long marks[MARK_MAX];
	unsigned lata[1][2], lnif[1][2];
	unsigned tag, dsk;
	int b, d, f, fd, howto, i, n;
	char *bname;
	void *dev;

	printf("\n");
	printf(">> NetBSD/sandpoint Boot, Revision %s\n", bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

	brdprop = brd_lookup(brdtype);
	printf("%s, cpu %u MHz, bus %u MHz, %dMB SDRAM\n", brdprop->verbose,
	    cpuclock / 1000000, busclock / 1000000, bi_mem.memsize >> 20);

	n = pcilookup(PCI_CLASS_IDE, lata, sizeof(lata)/sizeof(lata[0]));
	if (n == 0)
		n = pcilookup(PCI_CLASS_MISCSTORAGE, lata,
		    sizeof(lata)/sizeof(lata[0]));
	if (n == 0) {
		dsk = ~0;
		printf("no IDE found\n");
	}
	else {
		dsk = lata[0][1];
		pcidecomposetag(dsk, &b, &d, &f);
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

	if (dskdv_init(dsk, &dev) == 0 || disk_scan(dev) == 0)
		printf("no IDE/SATA device driver was found\n");

	if (netif_init(tag) == 0)
		printf("no NIC device driver was found\n");

	howto = RB_AUTOBOOT;		/* default is autoboot = 0 */

	/* get boot options and determine bootname */
	for (n = 1; n < argc; n++) {
		for (i = 0; i < sizeof(bootargs) / sizeof(bootargs[0]); i++) {
			if (strncasecmp(argv[n], bootargs[i].name,
			    strlen(bootargs[i].name)) == 0) {
				howto |= bootargs[i].value;
				break;
			}
		}
		if (i >= sizeof(bootargs) / sizeof(bootargs[0]))
			break;	/* break on first unknown string */
	}
	if (n >= argc)
		bname = BNAME_DEFAULT;
	else {
		bname = argv[n];
		if (check_bootname(bname) == 0) {
			printf("%s not a valid bootname\n", bname);
			goto loadfail;
		}
	}

	if ((fd = open(bname, 0)) < 0) {
		if (errno == ENOENT)
			printf("\"%s\" not found\n", bi_path.bootpath);
		goto loadfail;
	}
	printf("loading \"%s\" ", bi_path.bootpath);
	marks[MARK_START] = 0;
	if (fdloadfile(fd, marks, LOAD_KERNEL) < 0)
		goto loadfail;

	bootinfo = (void *)0x4000;
	bi_init(bootinfo);

	bi_add(&bi_cons, BTINFO_CONSOLE, sizeof(bi_cons));
	bi_add(&bi_mem, BTINFO_MEMORY, sizeof(bi_mem));
	bi_add(&bi_clk, BTINFO_CLOCK, sizeof(bi_clk));
	bi_add(&bi_path, BTINFO_BOOTPATH, sizeof(bi_path));
	bi_add(&bi_rdev, BTINFO_ROOTDEVICE, sizeof(bi_rdev));
	bi_add(&bi_fam, BTINFO_PRODFAMILY, sizeof(bi_fam));
	if (brdtype == BRD_SYNOLOGY) {
		/* need to set MAC address for Marvell-SKnet */
		bi_add(&bi_net, BTINFO_NET, sizeof(bi_net));
	}

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

  loadfail:
	printf("load failed. Restarting...\n");
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

void *
allocaligned(size_t size, size_t align)
{
	uint32_t p;

	if (align-- < 2)
		return alloc(size);
	p = (uint32_t)alloc(size + align);
	return (void *)((p + align) & ~align);
}

static int
check_bootname(char *s)
{
	/*
	 * nfs:
	 * nfs:<bootfile>
	 * tftp:
	 * tftp:<bootfile>
	 * wdN:<bootfile>
	 *
	 * net is a synonym of nfs.
	 */
	if (strncmp(s, "nfs:", 4) == 0 || strncmp(s, "net:", 4) == 0)
		return 1;
	if (strncmp(s, "tftp:", 5) == 0)
		return 1;
	if (s[0] == 'w' && s[1] == 'd'
	    && s[2] >= '0' && s[2] <= '3' && s[3] == ':') {
		return s[4] != '\0';
	}
	return 0;
}
