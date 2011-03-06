/* $NetBSD: main.c,v 1.8.2.2 2011/03/06 15:07:55 bouyer Exp $ */

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
struct btinfo_modulelist *btinfo_modulelist;
size_t btinfo_modulelist_size;

struct boot_module {
	char *bm_kmod;
	ssize_t bm_len;
	struct boot_module *bm_next;
};
struct boot_module *boot_modules;
char module_base[80];
uint32_t kmodloadp;
int modules_enabled = 0;

void module_add(char *);
void module_load(char *);
int module_open(struct boot_module *);

void main(int, char **, char *, char *);
extern char bootprog_name[], bootprog_rev[];

struct pcidev lata[2];
struct pcidev lnif[1];
struct pcidev lusb[3];
int nata, nnif, nusb;

int brdtype;
uint32_t busclock, cpuclock;

static int check_bootname(char *);
static int parse_cmdline(char **, int, char *, char *);
static int is_space(char);

#define	BNAME_DEFAULT "nfs:"
#define MAX_ARGS 10

void
main(int argc, char *argv[], char *bootargs_start, char *bootargs_end)
{
	struct brdprop *brdprop;
	unsigned long marks[MARK_MAX];
	char *new_argv[MAX_ARGS];
	int n, i, fd, howto;
	char *bname;

	printf("\n");
	printf(">> %s altboot, revision %s\n", bootprog_name, bootprog_rev);

	brdprop = brd_lookup(brdtype);
	printf(">> %s, cpu %u MHz, bus %u MHz, %dMB SDRAM\n", brdprop->verbose,
	    cpuclock / 1000000, busclock / 1000000, bi_mem.memsize >> 20);

	nata = pcilookup(PCI_CLASS_IDE, lata, 2);
	if (nata == 0)
		nata = pcilookup(PCI_CLASS_MISCSTORAGE, lata, 2);
	nnif = pcilookup(PCI_CLASS_ETH, lnif, 1);
	nusb = pcilookup(PCI_CLASS_USB, lusb, 3);

#ifdef DEBUG
	if (nata == 0)
		printf("No IDE/SATA found\n");
	else for (n = 0; n < nata; n++) {
		int b, d, f, bdf, pvd;
		bdf = lata[n].bdf;
		pvd = lata[n].pvd;
		pcidecomposetag(bdf, &b, &d, &f);
		printf("%04x.%04x DSK %02d:%02d:%02d\n",
		    PCI_VENDOR(pvd), PCI_PRODUCT(pvd), b, d, f);
	}
	if (nnif == 0)
		printf("no NET found\n");
	else {
		int b, d, f, bdf, pvd;
		bdf = lnif[0].bdf;
		pvd = lnif[0].pvd;
		pcidecomposetag(bdf, &b, &d, &f);
		printf("%04x.%04x NET %02d:%02d:%02d\n",
		    PCI_VENDOR(pvd), PCI_PRODUCT(pvd), b, d, f);
	}
	if (nusb == 0)
		printf("no USB found\n");
	else for (n = 0; n < nusb; n++) {
		int b, d, f, bdf, pvd;
		bdf = lusb[0].bdf;
		pvd = lusb[0].pvd;
		pcidecomposetag(bdf, &b, &d, &f);
		printf("%04x.%04x USB %02d:%02d:%02d\n",
		    PCI_VENDOR(pvd), PCI_PRODUCT(pvd), b, d, f);
	}
#endif

	pcisetup();
	pcifixup();

	if (dskdv_init(&lata[0]) == 0
	    || (nata == 2 && dskdv_init(&lata[1]) == 0))
		printf("IDE/SATA device driver was not found\n");

	if (netif_init(&lnif[0]) == 0)
		printf("no NET device driver was found\n");

	/*
	 * When argc is too big then it is probably a pointer, which could
	 * indicate that we were launched as a Linux kernel module using
	 * "bootm".
	 */
	if (argc > MAX_ARGS) {
		/* parse Linux bootargs */
		argv = new_argv;
		argc = parse_cmdline(argv, MAX_ARGS, bootargs_start,
		    bootargs_end);
	}

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
	close(fd);

	printf("entry=%p, ssym=%p, esym=%p\n",
	    (void *)marks[MARK_ENTRY],
	    (void *)marks[MARK_SYM],
	    (void *)marks[MARK_END]);

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

	if (modules_enabled) {
		module_add(fsmod);
		if (fsmod2 != NULL && strcmp(fsmod, fsmod2) != 0)
			module_add(fsmod2);
		kmodloadp = marks[MARK_END];
		btinfo_modulelist = NULL;
		module_load(bname);
		if (btinfo_modulelist != NULL && btinfo_modulelist->num > 0)
			bi_add(btinfo_modulelist, BTINFO_MODULELIST,
			    btinfo_modulelist_size);
	}

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

void
module_add(char *name)
{
	struct boot_module *bm, *bmp;

	while (*name == ' ' || *name == '\t')
		++name;

	bm = alloc(sizeof(struct boot_module) + strlen(name) + 1);
	if (bm == NULL) {
		printf("couldn't allocate module %s\n", name); 
		return; 
	}

	bm->bm_kmod = (char *)(bm + 1);
	bm->bm_len = -1;
	bm->bm_next = NULL;
	strcpy(bm->bm_kmod, name);
	if ((bmp = boot_modules) == NULL)
		boot_modules = bm;
	else {
		while (bmp->bm_next != NULL)
			bmp = bmp->bm_next;
		bmp->bm_next = bm;
	}
}

#define PAGE_SIZE	4096
#define alignpg(x)	(((x)+PAGE_SIZE-1) & ~(PAGE_SIZE-1))

void
module_load(char *kernel_path) 
{
	struct boot_module *bm;
	struct bi_modulelist_entry *bi;
	struct stat st;
	char *p; 
	int size, fd;

	strcpy(module_base, kernel_path);
	if ((p = strchr(module_base, ':')) == NULL)
		return; /* eeh?! */
	p += 1;
	size = sizeof(module_base) - (p - module_base);

	if (netbsd_version / 1000000 % 100 == 99) {
		/* -current */
		snprintf(p, size,
		    "/stand/sandpoint/%d.%d.%d/modules",
		    netbsd_version / 100000000,
		    netbsd_version / 1000000 % 100,
		    netbsd_version / 100 % 100);
	}
	 else if (netbsd_version != 0) {
		/* release */
		snprintf(p, size,
		    "/stand/sandpoint/%d.%d/modules",
		    netbsd_version / 100000000,
		    netbsd_version / 1000000 % 100);
	}

	/*
	 * 1st pass; determine module existence
	 */
	size = 0;
	for (bm = boot_modules; bm != NULL; bm = bm->bm_next) {
		fd = module_open(bm);
		if (fd == -1)
			continue;
		if (fstat(fd, &st) == -1 || st.st_size == -1) {
			printf("WARNING: couldn't stat %s\n", bm->bm_kmod);
			close(fd);
			continue;
		}
		bm->bm_len = (int)st.st_size;
		close(fd);
		size += sizeof(struct bi_modulelist_entry); 
	}
	if (size == 0)
		return;

	size += sizeof(struct btinfo_modulelist);
	btinfo_modulelist = alloc(size);
	if (btinfo_modulelist == NULL) {
		printf("WARNING: couldn't allocate module list\n");
		return;
	}
	btinfo_modulelist_size = size;
	btinfo_modulelist->num = 0;

	/*
	 * 2nd pass; load modules into memory
	 */
	kmodloadp = alignpg(kmodloadp);
	bi = (struct bi_modulelist_entry *)(btinfo_modulelist + 1);
	for (bm = boot_modules; bm != NULL; bm = bm->bm_next) {
		if (bm->bm_len == -1)
			continue; /* already found unavailable */
		fd = module_open(bm);
		printf("module \"%s\" ", bm->bm_kmod);
		size = read(fd, (char *)kmodloadp, SSIZE_MAX);
		if (size < bm->bm_len)
			printf("WARNING: couldn't load");
		else {
			snprintf(bi->kmod, sizeof(bi->kmod), bm->bm_kmod);
			bi->type = BI_MODULE_ELF;
			bi->len = size;
			bi->base = kmodloadp;
			btinfo_modulelist->num += 1;
			printf("loaded at 0x%08x size 0x%x", kmodloadp, size);
			kmodloadp += alignpg(size);
			bi += 1;
		}
		printf("\n");
		close(fd);
	}
	btinfo_modulelist->endpa = kmodloadp;
}

int
module_open(struct boot_module *bm)
{
	char path[80];
	int fd;

	snprintf(path, sizeof(path),
	    "%s/%s/%s.kmod", module_base, bm->bm_kmod, bm->bm_kmod);
	fd = open(path, 0);
	return fd;
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
	 * wd[N[P]]:<bootfile>
	 *
	 * net is a synonym of nfs.
	 */
	if (strncmp(s, "nfs:", 4) == 0 || strncmp(s, "net:", 4) == 0)
		return 1;
	if (strncmp(s, "tftp:", 5) == 0)
		return 1;
	if (s[0] == 'w' && s[1] == 'd') {
		s += 2;
		if (*s != ':' && *s >= '0' && *s <= '3') {
			++s;
			if (*s != ':' && *s >= 'a' && *s <= 'p')
				++s;
		}
		return *s == ':';
	}
	return 0;
}

static int
parse_cmdline(char **argv, int maxargc, char *p, char *end)
{
	int argc;

	argv[0] = "";
	for (argc = 1; argc < maxargc && p < end; argc++) {
		while (is_space(*p))
			p++;
		if (p >= end)
			break;
		argv[argc] = p;
		while (!is_space(*p) && p < end)
			p++;
		*p++ = '\0';
	}

	return argc;
}

static int
is_space(char c)
{
	return c > '\0' && c <= ' ';
}
