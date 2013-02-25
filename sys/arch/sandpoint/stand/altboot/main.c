/* $NetBSD: main.c,v 1.21.2.1 2013/02/25 00:28:55 tls Exp $ */

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
	{ "debug",	AB_DEBUG },
	{ "altboot",	-1 }
};

/* default PATA drive configuration is "10": single master on first channel */
static char *drive_config = "10";

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
extern char newaltboot[], newaltboot_end[];

struct pcidev lata[2];
struct pcidev lnif[2];
struct pcidev lusb[3];
int nata, nnif, nusb;

int brdtype;
uint32_t busclock, cpuclock;

static int check_bootname(char *);
static int input_cmdline(char **, int);
static int parse_cmdline(char **, int, char *, char *);
static int is_space(char);
#ifdef DEBUG
static void sat_test(void);
static void findflash(void);
#endif

#define	BNAME_DEFAULT "wd0:"
#define MAX_ARGS 10

void
main(int argc, char *argv[], char *bootargs_start, char *bootargs_end)
{
	unsigned long marks[MARK_MAX];
	struct brdprop *brdprop;
	char *new_argv[MAX_ARGS];
	char *bname;
	ssize_t len;
	int err, fd, howto, i, n;

	printf("\n>> %s altboot, revision %s\n", bootprog_name, bootprog_rev);

	brdprop = brd_lookup(brdtype);
	printf(">> %s, cpu %u MHz, bus %u MHz, %dMB SDRAM\n", brdprop->verbose,
	    cpuclock / 1000000, busclock / 1000000, bi_mem.memsize >> 20);

	nata = pcilookup(PCI_CLASS_IDE, lata, 2);
	if (nata == 0)
		nata = pcilookup(PCI_CLASS_RAID, lata, 2);
	if (nata == 0)
		nata = pcilookup(PCI_CLASS_MISCSTORAGE, lata, 2);
	if (nata == 0)
		nata = pcilookup(PCI_CLASS_SCSI, lata, 2);
	nnif = pcilookup(PCI_CLASS_ETH, lnif, 2);
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
	else for (n = 0; n < nnif; n++) {
		int b, d, f, bdf, pvd;
		bdf = lnif[n].bdf;
		pvd = lnif[n].pvd;
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

	/*
	 * When argc is too big then it is probably a pointer, which could
	 * indicate that we were launched as a Linux kernel module using
	 * "bootm".
	 */
	if (argc > MAX_ARGS) {
		if (argv != NULL) {
			/*
			 * initrd image was loaded:
			 * check if it contains a valid altboot command line
			 */
			char *p = (char *)argv;

			if (strncmp(p, "altboot:", 8) == 0) {
				*p = 0;
				for (p = p + 8; *p >= ' '; p++);
				argc = parse_cmdline(new_argv, MAX_ARGS,
				    ((char *)argv) + 8, p);
				argv = new_argv;
			} else
				argc = 0;	/* boot default */
		} else {
			/* parse standard Linux bootargs */
			argc = parse_cmdline(new_argv, MAX_ARGS,
			    bootargs_start, bootargs_end);
			argv = new_argv;
		}
	}

	/* look for a PATA drive configuration string under the arguments */
	for (n = 1; n < argc; n++) {
		if (strncmp(argv[n], "ide:", 4) == 0 &&
		    argv[n][4] >= '0' && argv[n][4] <= '2') {
			drive_config = &argv[n][4];
			break;
		}
	}

	/* intialize a disk driver */
	for (i = 0, n = 0; i < nata; i++)
		n += dskdv_init(&lata[i]);
	if (n == 0)
		printf("IDE/SATA device driver was not found\n");

	/* initialize a network interface */
	for (n = 0; n < nnif; n++)
		if (netif_init(&lnif[n]) != 0)
			break;
	if (n >= nnif)
		printf("no NET device driver was found\n");

	/* wait 2s for user to enter interactive mode */
	for (n = 200; n >= 0; n--) {
		if (n % 100 == 0)
			printf("\rHit any key to enter interactive mode: %d",
			    n / 100);
		if (tstchar()) {
#ifdef DEBUG
			unsigned c;

			c = toupper(getchar());
			if (c == 'C') {
				/* controller test terminal */
				sat_test();
				n = 200;
				continue;
			}
			else if (c == 'F') {
				/* find strings in Flash ROM */
				findflash();
				n = 200;
				continue;
			}
#else
			(void)getchar();
#endif
			/* enter command line */
			argv = new_argv;
			argc = input_cmdline(argv, MAX_ARGS);
			break;
		}
		delay(10000);
	}
	putchar('\n');

	howto = RB_AUTOBOOT;		/* default is autoboot = 0 */

	/* get boot options and determine bootname */
	for (n = 1; n < argc; n++) {
		if (strncmp(argv[n], "ide:", 4) == 0)
			continue; /* ignore drive configuration argument */

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

	/*
	 * If no device name is given, we construct a list of drives
	 * which have valid disklabels.
	 */
	if (n >= argc) {
		n = 0;
		argc = 0;
		argv = alloc(MAX_UNITS * (sizeof(char *) + sizeof("wdN:")));
		bname = (char *)(argv + MAX_UNITS);
		for (i = 0; i < MAX_UNITS; i++) {
			if (!dlabel_valid(i))
				continue;
			sprintf(bname, "wd%d:", i);
			argv[argc++] = bname;
			bname += sizeof("wdN:");
		}
		/* use default drive if no valid disklabel is found */
		if (argc == 0) {
			argc = 1;
			argv[0] = BNAME_DEFAULT;
		}
	}

	/* try to boot off kernel from the drive list */
	while (n < argc) {
		bname = argv[n++];

		if (check_bootname(bname) == 0) {
			printf("%s not a valid bootname\n", bname);
			continue;
		}

		if ((fd = open(bname, 0)) < 0) {
			if (errno == ENOENT)
				printf("\"%s\" not found\n", bi_path.bootpath);
			continue;
		}
		printf("loading \"%s\" ", bi_path.bootpath);
		marks[MARK_START] = 0;

		if (howto == -1) {
			/* load another altboot binary and replace ourselves */
			len = read(fd, (void *)0x100000, 0x1000000 - 0x100000);
			if (len == -1)
				goto loadfail;
			close(fd);
			netif_shutdown_all();

			memcpy((void *)0xf0000, newaltboot,
			    newaltboot_end - newaltboot);
			__syncicache((void *)0xf0000,
			    newaltboot_end - newaltboot);
			printf("Restarting...\n");
			run((void *)1, argv, (void *)0x100000, (void *)len,
			    (void *)0xf0000);
		}

		err = fdloadfile(fd, marks, LOAD_KERNEL);
		close(fd);
		if (err < 0)
			continue;

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
		if (brdtype == BRD_SYNOLOGY || brdtype == BRD_DLINKDSM) {
			/* need to pass this MAC address to kernel */
			bi_add(&bi_net, BTINFO_NET, sizeof(bi_net));
		}

		if (modules_enabled) {
			if (fsmod != NULL)
				module_add(fsmod);
			kmodloadp = marks[MARK_END];
			btinfo_modulelist = NULL;
			module_load(bname);
			if (btinfo_modulelist != NULL &&
			    btinfo_modulelist->num > 0)
				bi_add(btinfo_modulelist, BTINFO_MODULELIST,
				    btinfo_modulelist_size);
		}

		launchfixup();
		netif_shutdown_all();

		__syncicache((void *)marks[MARK_ENTRY],
		    (u_int)marks[MARK_SYM] - (u_int)marks[MARK_ENTRY]);

		run((void *)marks[MARK_SYM], (void *)marks[MARK_END],
		    (void *)howto, bootinfo, (void *)marks[MARK_ENTRY]);

		/* should never come here */
		printf("exec returned. Restarting...\n");
		_rtt();
	}
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

/*
 * Return the drive configuration for the requested channel 'ch'.
 * Channel 2 is the first channel of the next IDE controller.
 * 0: for no drive present on channel
 * 1: for master drive present on channel, no slave
 * 2: for master and slave drive present
 */
int
get_drive_config(int ch)
{
	if (drive_config != NULL) {
		if (strlen(drive_config) <= ch)
			return 0;	/* an unspecified channel is unused */
		if (drive_config[ch] >= '0' && drive_config[ch] <= '2')
			return drive_config[ch] - '0';
	}
	return -1;
}

void *
allocaligned(size_t size, size_t align)
{
	uint32_t p;

	if (align-- < 2)
		return alloc(size);
	p = (uint32_t)alloc(size + align);
	return (void *)((p + align) & ~align);
}

static int hex2nibble(char c)
{

	if (c >= 'a')
		c &= ~0x20;
	if (c >= 'A' && c <= 'F')
		c -= 'A' - ('9' + 1);
	else if (c < '0' || c > '9')
		return -1;

	return c - '0';
}

uint32_t
read_hex(const char *s)
{
	int n;
	uint32_t val;

	val = 0;
	while ((n = hex2nibble(*s++)) >= 0)
		val = (val << 4) | n;
	return val;
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
	 * mem:<address>
	 *
	 * net is a synonym of nfs.
	 */
	if (strncmp(s, "nfs:", 4) == 0 || strncmp(s, "net:", 4) == 0 ||
	    strncmp(s, "tftp:", 5) == 0 || strncmp(s, "mem:", 4) == 0)
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

static int input_cmdline(char **argv, int maxargc)
{
	char *cmdline;

	printf("\nbootargs> ");
	cmdline = alloc(256);
	gets(cmdline);

	return parse_cmdline(argv, maxargc, cmdline,
	    cmdline + strlen(cmdline));
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

#ifdef DEBUG
static void
findflash(void)
{
	char buf[256];
	int i, n;
	unsigned char c, *p;

	for (;;) {
		printf("\nfind> ");
		gets(buf);
		if (tolower((unsigned)buf[0]) == 'x')
			break;
		for (i = 0, n = 0, c = 0; buf[i]; i++) {
			c <<= 4;
			c |= hex2nibble(buf[i]);
			if (i & 1)
				buf[n++] = c;
		}
		printf("Searching for:");
		for (i = 0; i < n; i++)
			printf(" %02x", buf[i]);
		printf("\n");
		for (p = (unsigned char *)0xff000000;
		     p <= (unsigned char *)(0xffffffff-n); p++) {
			for (i = 0; i < n; i++) {
				if (p[i] != buf[i])
					break;
			}
			if (i >= n)
				printf("Found at %08x\n", (unsigned)p);
		}
	}
}

static void
sat_test(void)
{
	char buf[1024];
	int i, j, n, pos;
	unsigned char c;

	putchar('\n');
	for (;;) {
		do {
			for (pos = 0; pos < 1024 && sat_tstch() != 0; pos++)
				buf[pos] = sat_getch();
			if (pos > 1023)
				break;
			delay(100000);
		} while (sat_tstch());

		for (i = 0; i < pos; i += 16) {
			if ((n = i + 16) > pos)
				n = pos;
			for (j = 0; j < n; j++)
				printf("%02x ", (unsigned)buf[i + j]);
			for (; j < 16; j++)
				printf("   ");
			putchar('\"');
			for (j = 0; j < n; j++) {
				c = buf[i + j];
				putchar((c >= 0x20 && c <= 0x7e) ? c : '.');
			}
			printf("\"\n");
		}

		printf("controller> ");
		gets(buf);
		if (buf[0] == '*' && buf[1] == 'X')
			break;

		if (buf[0] == '0' && tolower((unsigned)buf[1]) == 'x') {
			for (i = 2, n = 0, c = 0; buf[i]; i++) {
				c <<= 4;
				c |= hex2nibble(buf[i]);
				if (i & 1)
					buf[n++] = c;
			}
		} else
			n = strlen(buf);

		if (n > 0)
			sat_write(buf, n);
	}
}
#endif
