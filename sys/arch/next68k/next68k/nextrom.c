/*	$NetBSD: nextrom.c,v 1.25.38.1 2018/03/15 09:12:04 pgoyette Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nextrom.c,v 1.25.38.1 2018/03/15 09:12:04 pgoyette Exp $");

#include "opt_ddb.h"
#include "opt_serial.h"

#include <sys/types.h>
#include <machine/cpu.h>

#include <next68k/next68k/seglist.h>
#include <next68k/next68k/nextrom.h>

#ifdef DDB
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#define ELFSIZE 32
#include <sys/exec_elf.h>
#endif

void    next68k_bootargs(unsigned char **);

int mon_getc(void);
int mon_putc(int);

extern char etext[], edata[], end[];
extern int nsym;
extern char *ssym, *esym;

volatile struct mon_global *mg;


#define	MON(type, off)	(*(type *)((u_int) (mg) + off))

#define RELOC(v, t)	(*((t *)((u_int)&(v) + NEXT_RAMBASE)))

#define	MONRELOC(type, off) \
    (*(volatile type *)((u_int)RELOC(mg, volatile struct mon_global *) + off))


typedef int (*getcptr)(void);
typedef int (*putcptr)(int);

/*
 * Print a string on the rom console before the MMU is turned on
 */

/* #define DISABLE_ROM_PRINT 1 */

#ifdef DISABLE_ROM_PRINT
#define ROM_PUTC(c)  /* nop */
#define ROM_PUTS(xs) /* nop */
#define ROM_PUTX(v)  /* nop */
#else

#define ROM_PUTC(c) \
	(*MONRELOC(putcptr, MG_putc))(c)
#define ROM_PUTS(xs) \
	do {								\
		volatile const char *_s = xs + NEXT_RAMBASE;		\
		while (_s && *_s)					\
			(*MONRELOC(putcptr, MG_putc))(*_s++);		\
	} while (/* CONSTCOND */0)

/* Print a hex byte on the rom console */

#if 1
static char romprint_hextable[] = "0123456789abcdef@";
#define ROM_PUTX(v)							\
	do {								\
		(*MONRELOC(putcptr, MG_putc))				\
		    (RELOC(romprint_hextable[((v)>>4)&0xf], char));	\
		(*MONRELOC(putcptr, MG_putc))				\
		    (RELOC(romprint_hextable[(v)&0xf], char));		\
	} while (/* CONSTCOND */0);
#else
#define lookup_hex(v)  ((v) >9 ? ('a' + (v) - 0xa) : ('0' + (v)))
#define ROM_PUTX(v) \
	do {								\
		(*MONRELOC(putcptr, MG_putc))				\
		    (lookup_hex(((v) >> 4) & 0xf));			\
		(*MONRELOC(putcptr, MG_putc))				\
		    (lookup_hex((v) & 0xf));				\
	} while (/* CONSTCOND */0);
#endif
#endif

uint8_t rom_enetaddr[6];
uint8_t rom_boot_dev[20];
uint8_t rom_boot_arg[20];
uint8_t rom_boot_info[20];
uint8_t rom_boot_file[20];
uint8_t rom_bootfile[MG_boot_how - MG_bootfile];
char rom_machine_type;

uint8_t *rom_return_sp;
u_int rom_mon_stack;
uint8_t rom_image[0x2000];
paddr_t rom_image_base;
u_int rom_vbr;
u_int rom_intrmask;
u_int rom_intrstat;

paddr_t rom_reboot_vect;

int turbo;

void
next68k_bootargs(unsigned char **args)
{
#ifdef DDB
	int i;
	Elf_Ehdr *ehdr;
	Elf_Shdr *shp;
	vaddr_t minsym, maxsym;
	char *reloc_end;
	const char *reloc_elfmag;
#endif

	RELOC(rom_return_sp, uint8_t *) = args[0];
	RELOC(mg, char *) = args[1];

	ROM_PUTS("Welcome to NetBSD/next68k\r\n");

#ifdef DDB

	/*
	 * Check the ELF headers.
	 */

	reloc_end = end + NEXT_RAMBASE;
#pragma GCC push_options
#pragma GCC diagnostic ignored "-Warray-bounds"
	reloc_elfmag = ELFMAG + NEXT_RAMBASE;
#pragma GCC pop_options
	ehdr = (void *)reloc_end;

	for (i = 0; i < SELFMAG; i++) {
		if (ehdr->e_ident[i] != reloc_elfmag[i]) {
			ROM_PUTS("save_symtab: bad ELF magic\n");
			goto ddbdone;
		}
	}
	if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
		ROM_PUTS("save_symtab: bad ELF magic\n");
		goto ddbdone;
	}

	/*
	 * Find the end of the symbols and strings.
	 */

	maxsym = 0;
	minsym = ~maxsym;
	shp = (Elf_Shdr *)(reloc_end + ehdr->e_shoff);
	for (i = 0; i < ehdr->e_shnum; i++) {
		if (shp[i].sh_type != SHT_SYMTAB &&
		    shp[i].sh_type != SHT_STRTAB) {
			continue;
		}
		minsym = MIN(minsym, (vaddr_t)reloc_end + shp[i].sh_offset);
		maxsym = MAX(maxsym, (vaddr_t)reloc_end + shp[i].sh_offset +
			     shp[i].sh_size);
	}
	RELOC(nsym, int) = 1;
	RELOC(ssym, char *) = end;
	RELOC(esym, char *) = (char *)maxsym - NEXT_RAMBASE;

	ROM_PUTS("nsym ");
	ROM_PUTX(RELOC(nsym, int));
	ROM_PUTS(" ssym ");
	ROM_PUTX((vaddr_t)RELOC(ssym, char *));
	ROM_PUTS(" esym ");
	ROM_PUTX((vaddr_t)RELOC(esym, char *));
	ROM_PUTS("\r\n");

 ddbdone:
#endif

	ROM_PUTS("Constructing the segment list...\r\n");

	ROM_PUTS("machine type = 0x");
	ROM_PUTX(MONRELOC(char, MG_machine_type));
	ROM_PUTS("\r\nboard rev = 0x");
	ROM_PUTX(MONRELOC(char, MG_board_rev));
	ROM_PUTS("\r\ndmachip = 0x");
	ROM_PUTX(MONRELOC(int, MG_dmachip) >> 24 & 0xff);
	ROM_PUTX(MONRELOC(int, MG_dmachip) >> 16 & 0xff);
	ROM_PUTX(MONRELOC(int, MG_dmachip) >>  8 & 0xff);
	ROM_PUTX(MONRELOC(int, MG_dmachip) >>  0 & 0xff);
	ROM_PUTS("\r\ndiskchip = 0x");
	ROM_PUTX(MONRELOC(int, MG_diskchip) >> 24 & 0xff);
	ROM_PUTX(MONRELOC(int, MG_diskchip) >> 16 & 0xff);
	ROM_PUTX(MONRELOC(int, MG_diskchip) >>  8 & 0xff);
	ROM_PUTX(MONRELOC(int, MG_diskchip) >>  0 & 0xff);
	ROM_PUTS("\r\n");


	/* Construct the segment list */
	{
		u_int msize16;
		u_int msize4;
		u_int msize1;
		int ix;
		int j = 0;
		char mach;

		if (MONRELOC(char, MG_machine_type) == NeXT_X15) {
			msize16 = 0x1000000;
			msize4  =  0x400000;
			msize1  =  0x100000;
			ROM_PUTS("Looks like a NeXT_X15\r\n");
		} else if (MONRELOC(char, MG_machine_type) == NeXT_WARP9C) {
			msize16 = 0x800000;
			msize4  = 0x200000;
			msize1  =  0x80000;				/* ? */
			ROM_PUTS("Looks like a NeXT_WARP9C\r\n");
		} else if (MONRELOC(char, MG_machine_type) == NeXT_WARP9) {
			msize16 = 0x1000000;
			msize4  =  0x400000;
			msize1  =  0x100000;
			ROM_PUTS("Looks like a NeXT_WARP9\r\n");
		} else if (MONRELOC(char, MG_machine_type) == NeXT_TURBO_COLOR)
		    {
			msize16 = 0x2000000;
			msize4  =  0x800000;
			msize1  =  0x200000;
			ROM_PUTS("Looks like a NeXT_TURBO_COLOR\r\n");
		} else if (MONRELOC(char, MG_machine_type) == NeXT_TURBO_MONO) {
			msize16 = 0x2000000;
			msize4  =  0x800000;
			msize1  =  0x200000;
			ROM_PUTS("Looks like a NeXT_TURBO_MONO\r\n");
		} else {
			msize16 = 0x100000;
			msize4  = 0x100000;
			msize1  = 0x100000;
			ROM_PUTS("Unrecognized machine_type\r\n");
		}

		mach = MONRELOC(char, MG_machine_type);
		RELOC(rom_machine_type, char) = mach;
		if (mach == NeXT_TURBO_MONO || mach == NeXT_TURBO_COLOR)
			RELOC(turbo, int) = 1;
		else
			RELOC(turbo, int) = 0;

		for (ix = 0; ix < N_SIMM; ix++) {

			ROM_PUTS("Memory bank 0x");
			ROM_PUTX(ix);
			ROM_PUTS(" has value 0x");
			ROM_PUTX(MONRELOC(char, MG_simm + ix))
			ROM_PUTS("\r\n");

			if ((MONRELOC(char, MG_simm+ix) & SIMM_SIZE) !=
			    SIMM_SIZE_EMPTY) {
				RELOC(phys_seg_list[j].ps_start, paddr_t) =
				    NEXT_RAMBASE + (ix * msize16);
			}
			if ((MONRELOC(char, MG_simm + ix) & SIMM_SIZE) ==
			    SIMM_SIZE_16MB) {
				RELOC(phys_seg_list[j].ps_end, paddr_t) =
				    RELOC(phys_seg_list[j].ps_start, paddr_t) +
				    msize16;
				j++;
			}
			if ((MONRELOC(char, MG_simm + ix) & SIMM_SIZE) ==
			    SIMM_SIZE_4MB) {
				RELOC(phys_seg_list[j].ps_end, paddr_t) =
				    RELOC(phys_seg_list[j].ps_start, paddr_t) +
				    msize4;
				j++;
			}
			if ((MONRELOC(char, MG_simm+ix) & SIMM_SIZE) ==
			    SIMM_SIZE_1MB) {
				RELOC(phys_seg_list[j].ps_end, paddr_t) =
				    RELOC(phys_seg_list[j].ps_start, paddr_t) +
				    msize1;
				j++;
			}
		}

		/*
		 * The NeXT ROM or something appears to reserve the very
		 * top of memory
		 */
		RELOC(phys_seg_list[j - 1].ps_end, paddr_t) -= 0x2000;
		RELOC(rom_image_base, paddr_t) =
		    RELOC(phys_seg_list[j - 1].ps_end, paddr_t);

		/* pmap is unhappy if it is not null terminated */
		for (; j < MAX_PHYS_SEGS; j++) {
			RELOC(phys_seg_list[j].ps_start, paddr_t) = 0;
			RELOC(phys_seg_list[j].ps_end, paddr_t) = 0;
		}
	}

	{
		int j;
		ROM_PUTS("Memory segments found:\r\n");
		for (j = 0; RELOC(phys_seg_list[j].ps_start, paddr_t); j++) {
			ROM_PUTS("\t0x");
			ROM_PUTX((RELOC(phys_seg_list[j].ps_start, paddr_t)
			    >> 24 ) & 0xff);
			ROM_PUTX((RELOC(phys_seg_list[j].ps_start, paddr_t)
			    >> 16) & 0xff);
			ROM_PUTX((RELOC(phys_seg_list[j].ps_start, paddr_t)
			    >>  8) & 0xff);
			ROM_PUTX((RELOC(phys_seg_list[j].ps_start, paddr_t)
			    >>  0) & 0xff);
			ROM_PUTS(" - 0x");
			ROM_PUTX((RELOC(phys_seg_list[j].ps_end, paddr_t)
			    >> 24) & 0xff);
			ROM_PUTX((RELOC(phys_seg_list[j].ps_end, paddr_t)
			    >> 16) & 0xff);
			ROM_PUTX((RELOC(phys_seg_list[j].ps_end, paddr_t)
			    >>  8) & 0xff);
			ROM_PUTX((RELOC(phys_seg_list[j].ps_end, paddr_t)
			    >>  0) & 0xff);
			ROM_PUTS("\r\n");
		}
	}

	/*
	 * Read the ethernet address from rom, this should be done later
	 * in device driver somehow.
	 */
	{
		int j;
		ROM_PUTS("Ethernet address: ");
		for (j = 0; j < 6; j++) {
			RELOC(rom_enetaddr[j], uint8_t) =
			    MONRELOC(uint8_t *, MG_clientetheraddr)[j];
			ROM_PUTX(RELOC(rom_enetaddr[j], uint8_t));
			if (j < 5)
				ROM_PUTS(":");
		}
		ROM_PUTS("\r\n");
	}

	/*
	 * Read the boot args
	 */
	{
		int j;
		for (j = 0; j < sizeof(rom_bootfile); j++) {
			RELOC(rom_bootfile[j], uint8_t) =
			    MONRELOC(uint8_t, MG_bootfile + j);
		}

		ROM_PUTS("rom bootdev: ");
		for (j = 0; j < sizeof(rom_boot_dev); j++) {
			RELOC(rom_boot_dev[j], uint8_t) =
			    MONRELOC(uint8_t *, MG_boot_dev)[j];
			ROM_PUTC(RELOC(rom_boot_dev[j], uint8_t));
			if (MONRELOC(uint8_t *, MG_boot_dev)[j] == '\0')
				break;
		}
		RELOC(rom_boot_dev[sizeof(rom_boot_dev) - 1], uint8_t) = 0;

		ROM_PUTS("\r\nrom bootarg: ");
		for (j = 0; j < sizeof(rom_boot_arg); j++) {
			RELOC(rom_boot_arg[j], uint8_t) =
			    MONRELOC(uint8_t *, MG_boot_arg)[j];
			ROM_PUTC(RELOC(rom_boot_arg[j], uint8_t));
			if (MONRELOC(uint8_t *, MG_boot_arg)[j] == '\0')
				break;
		}
		RELOC(rom_boot_arg[sizeof(rom_boot_arg) - 1], uint8_t) = 0;

		ROM_PUTS("\r\nrom bootinfo: ");
		for (j = 0; j < sizeof(rom_boot_info); j++) {
			RELOC(rom_boot_info[j], uint8_t) =
			    MONRELOC(uint8_t *, MG_boot_info)[j];
			ROM_PUTC(RELOC(rom_boot_info[j], uint8_t));
			if (MONRELOC(uint8_t *, MG_boot_info)[j] == '\0')
				break;
		}
		RELOC(rom_boot_info[sizeof(rom_boot_info) - 1], uint8_t) = 0;

		ROM_PUTS("\r\nrom bootfile: ");
		for (j = 0; j < sizeof(rom_boot_file); j++) {
			RELOC(rom_boot_file[j], uint8_t) =
			    MONRELOC(uint8_t *, MG_boot_file)[j];
			ROM_PUTC(RELOC(rom_boot_file[j], uint8_t));
			if (MONRELOC(uint8_t *, MG_boot_file)[j] == '\0')
				break;
		}
		RELOC(rom_boot_file[sizeof(rom_boot_file) - 1], uint8_t) = 0;
		ROM_PUTS("\r\n");

		RELOC(rom_mon_stack, u_int) = MONRELOC(u_int, MG_mon_stack);
		RELOC(rom_vbr, u_int) = MONRELOC(u_int, MG_vbr);
		RELOC(rom_reboot_vect, paddr_t) =
		    MONRELOC(paddr_t *, MG_vbr)[45]; /* trap #13 */

		for (j = 0; j < sizeof(rom_image); j++) {
			RELOC(rom_image[j], uint8_t) =
			    *(uint8_t *)(RELOC(rom_image_base, paddr_t) + j);
		}
	}

	RELOC(rom_intrmask, u_int) = MONRELOC(u_int, MG_intrmask);
	RELOC(rom_intrstat, u_int) = MONRELOC(u_int, MG_intrstat);
	ROM_PUTS("intrmask: ");
	ROM_PUTX((RELOC(rom_intrmask, u_int) >> 24) & 0xff);
	ROM_PUTX((RELOC(rom_intrmask, u_int) >> 16) & 0xff);
	ROM_PUTX((RELOC(rom_intrmask, u_int) >>  8) & 0xff);
	ROM_PUTX((RELOC(rom_intrmask, u_int) >>  0) & 0xff);
	ROM_PUTS("\r\nintrstat: ");
	ROM_PUTX((RELOC(rom_intrstat, u_int) >> 24) & 0xff);
	ROM_PUTX((RELOC(rom_intrstat, u_int) >> 16) & 0xff);
	ROM_PUTX((RELOC(rom_intrstat, u_int) >>  8) & 0xff);
	ROM_PUTX((RELOC(rom_intrstat, u_int) >>  0) & 0xff);
	ROM_PUTS("\r\n");
#if 0
	RELOC(rom_intrmask, u_int) = 0x02007800;
	RELOC(rom_intrstat, u_int) = 0x02007000;
#endif

#ifdef SERCONSOLE
	ROM_PUTS("Check serial port A for console.\r\n");
#endif
}
