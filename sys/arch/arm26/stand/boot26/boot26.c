/*	$NetBSD: boot26.c,v 1.2 2001/07/28 13:49:25 bjh21 Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000, 2001 Ben Harris
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <riscoscalls.h>
#include <sys/boot_flag.h>
#include <machine/boot.h>
#include <machine/memcreg.h>

extern const char bootprog_rev[];
extern const char bootprog_name[];
extern const char bootprog_date[];
extern const char bootprog_maker[];

int debug = 1;

enum pgstatus {	FREE, USED_RISCOS, USED_KERNEL, USED_BOOT };

int nbpp;
struct os_mem_map_request *pginfo;
enum pgstatus *pgstatus;

u_long marks[MARK_MAX];

char fbuf[80];

void get_mem_map(struct os_mem_map_request *, enum pgstatus *, int);
int vdu_var(int);

extern void start_kernel(struct bootconfig *, u_int, u_int);

int
main(int argc, char **argv)
{
	int npages, howto;
	int i, j;
	char *file;
	struct bootconfig bootconfig;
	int crow;
	int ret;

	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

	os_read_mem_map_info(&nbpp, &npages);
	if (debug)
		printf("Machine has %d pages of %d KB each.  "
		    "Total RAM: %d MB\n", npages, nbpp >> 10,
		    (npages * nbpp) >> 20);

	/* Need one extra for teminator in OS_ReadMemMapEntries. */
	pginfo = alloc((npages + 1) * sizeof(*pginfo));
	if (pginfo == NULL)
		panic("cannot alloc pginfo array");
	memset(pginfo, 0, npages * sizeof(*pginfo));
	pgstatus = alloc(npages * sizeof(*pgstatus));
	if (pgstatus == NULL)
		panic("cannot alloc pgstatus array");
	memset(pgstatus, 0, npages * sizeof(*pgstatus));

	get_mem_map(pginfo, pgstatus, npages);

	howto = 0;
	file = NULL;
	for (i = 1; i < argc; i++)
		if (argv[i][0] == '-')
			for (j = 1; argv[i][j]; j++)
				BOOT_FLAG(argv[i][j], howto);
		else
			if (file)
				panic("Too many files!");
			else
				file = argv[i];
	if (file == NULL) {
		if (howto & RB_ASKNAME) {
			printf("boot: ");
			gets(fbuf);
			file = fbuf;
		} else
			file = "netbsd";
	}
	printf("Booting %s (howto = 0x%x)\n", file, howto);

	ret = loadfile(file, marks, LOAD_KERNEL);
	if (ret == -1)
		panic("Kernel load failed");
	close(ret);

	printf("Starting at 0x%lx\n", marks[MARK_ENTRY]);

	memset(&bootconfig, 0, sizeof(bootconfig));
	bootconfig.magic = BOOT_MAGIC;
	bootconfig.version = 0;
	bootconfig.boothowto = howto;
	bootconfig.bootdev = -1;
	bootconfig.ssym = (caddr_t)marks[MARK_SYM] - MEMC_PHYS_BASE;
	bootconfig.esym = (caddr_t)marks[MARK_END] - MEMC_PHYS_BASE;
	bootconfig.nbpp = nbpp;
	bootconfig.npages = npages;
	bootconfig.freebase = (caddr_t)marks[MARK_END] - MEMC_PHYS_BASE;
	bootconfig.xpixels = vdu_var(os_MODEVAR_XWIND_LIMIT) + 1;
	bootconfig.ypixels = vdu_var(os_MODEVAR_YWIND_LIMIT) + 1;
	bootconfig.bpp = 1 << vdu_var(os_MODEVAR_LOG2_BPP);
	bootconfig.screenbase = (caddr_t)vdu_var(os_VDUVAR_DISPLAY_START) +
	    vdu_var(os_VDUVAR_TOTAL_SCREEN_SIZE) - MEMC_PHYS_BASE;
	bootconfig.screensize = vdu_var(os_VDUVAR_TOTAL_SCREEN_SIZE);
	os_byte(osbyte_OUTPUT_CURSOR_POSITION, 0, 0, NULL, &crow);
	bootconfig.cpixelrow = crow * vdu_var(os_VDUVAR_TCHAR_SPACEY);

	if (bootconfig.bpp < 8)
		printf("WARNING: Current screen mode has fewer than eight "
							"bits per pixel.\n"
		    "         Console display may not work correctly "
		    					"(or at all).\n");
	/* Tear down RISC OS... */

	/* NetBSD will want the cache off initially. */
	xcache_control(0, 0, NULL);

	/* Dismount all filesystems. */
	xosfscontrol_shutdown();

	/* Ask device drivers to reset devices. */
	service_pre_reset();

	/* Disable interrupts. */
	os_int_off();

	start_kernel(&bootconfig, marks[MARK_ENTRY], 0x02090000);

	return 0;
}

void
get_mem_map(struct os_mem_map_request *pginfo, enum pgstatus *pgstatus,
    int npages)
{
	int i;

	for (i = 0; i < npages; i++)
		pginfo[i].page_no = i;
	pginfo[npages].page_no = -1;
	os_read_mem_map_entries(pginfo);

	if (debug)
		printf("--------/-------/-------/-------\n");
	for (i = 0; i < npages; i++) {
		pgstatus[i] = USED_RISCOS;
		if (pginfo[i].access == os_AREA_ACCESS_NONE) {
			if (debug) printf(".");
		} else {
			if (pginfo[i].map < (caddr_t)0x0008000) {
				if (debug) printf("0");
			} else if (pginfo[i].map < (caddr_t)HIMEM) {
				pgstatus[i] = USED_BOOT;
				if (debug) printf("+");
			} else if (pginfo[i].map < (caddr_t)0x1000000) {
				if (pginfo[i].access ==
				    os_AREA_ACCESS_READ_WRITE) {
					pgstatus[i] = FREE;
					if (debug) printf("*");
				} else {
					if (debug) printf("a");
				}
			} else if (pginfo[i].map < (caddr_t)0x1400000) {
				if (debug) printf("d");
			} else if (pginfo[i].map < (caddr_t)0x1800000) {
				if (debug) printf("s");
			} else if (pginfo[i].map < (caddr_t)0x1c00000) {
				if (debug) printf("m");
			} else if (pginfo[i].map < (caddr_t)0x1e00000) {
				if (debug) printf("h");
			} else if (pginfo[i].map < (caddr_t)0x1f00000) {
				if (debug) printf("f");
			} else if (pginfo[i].map < (caddr_t)0x2000000) {
				if (debug) printf("S");
			}
		}
		if (i % 32 == 31 && debug)
			printf("\n");
	}
}

ssize_t
boot26_read(int f, void *addr, size_t size)
{
	int ppn;
	caddr_t fragaddr;
	size_t fragsize;
	ssize_t retval, total;

	total = 0;
	while (size > 0) {
		ppn = ((caddr_t)addr - MEMC_PHYS_BASE) / nbpp;
		if (pgstatus[ppn] != FREE)
			panic("Page %d not free", ppn);
		fragaddr = pginfo[ppn].map + ((u_int)addr % nbpp);
		fragsize = nbpp - ((u_int)addr % nbpp);
		if (fragsize > size)
			fragsize = size;
		retval = read(f, fragaddr, fragsize);
		if (retval < 0)
			return retval;
		total += retval;
		if (retval < fragsize)
			return total;
		addr += fragsize;
		size -= fragsize;
	}
	return total;
}

void *
boot26_memcpy(void *dst, const void *src, size_t size)
{
	int ppn;
	caddr_t fragaddr;
	size_t fragsize;
	void *addr = dst;

	while (size > 0) {
		ppn = ((caddr_t)addr - MEMC_PHYS_BASE) / nbpp;
		if (pgstatus[ppn] != FREE)
			panic("Page %d not free", ppn);
		fragaddr = pginfo[ppn].map + ((u_int)addr % nbpp);
		fragsize = nbpp - ((u_int)addr % nbpp);
		if (fragsize > size)
			fragsize = size;
		memcpy(fragaddr, src, fragsize);
		addr += fragsize;
		src += fragsize;
		size -= fragsize;
	}
	return dst;
}

void *
boot26_memset(void *dst, int c, size_t size)
{
	int ppn;
	caddr_t fragaddr;
	size_t fragsize;
	void *addr = dst;

	while (size > 0) {
		ppn = ((caddr_t)addr - MEMC_PHYS_BASE) / nbpp;
		if (pgstatus[ppn] != FREE)
			panic("Page %d not free", ppn);
		fragaddr = pginfo[ppn].map + ((u_int)addr % nbpp);
		fragsize = nbpp - ((u_int)addr % nbpp);
		if (fragsize > size)
			fragsize = size;
		memset(fragaddr, c, fragsize);
		addr += fragsize;
		size -= fragsize;
	}
	return dst;
}

int
vdu_var(int var)
{
	int varlist[2], vallist[2];

	varlist[0] = var;
	varlist[1] = -1;
	os_read_vdu_variables(varlist, vallist);
	return vallist[0];
}
