/*	$NetBSD: nextrom.c,v 1.12 2001/05/12 22:35:30 chs Exp $	*/
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Darrin B. Jewell
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include "opt_ddb.h"

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

void    next68k_bootargs __P((unsigned char *args[]));

int mon_getc(void);
int mon_putc(int c);

extern char etext[], edata[], end[];
int nsym;
char *ssym, *esym;

volatile struct mon_global *mg;


#define	MON(type, off) (*(type *)((u_int) (mg) + off))

#define RELOC(v, t)	(*((t *)((u_int)&(v) + NEXT_RAMBASE)))

#define	MONRELOC(type, off) \
     (*(volatile type *)((u_int) RELOC(mg,volatile struct mon_global *) + off))


typedef int (*getcptr)(void);
typedef int (*putcptr)(int);

/*
 * Print a string on the rom console before the MMU is turned on
 */

/* #define DISABLE_ROM_PRINT 1 */

#ifdef DISABLE_ROM_PRINT
#define ROM_PUTS(xs) /* nop */
#define ROM_PUTX(v)  /* nop */
#else

#define ROM_PUTS(xs) \
  do { volatile char *_s = xs + NEXT_RAMBASE; \
     while(_s && *_s) (*MONRELOC(putcptr,MG_putc))(*_s++); \
	} while(0)

/* Print a hex byte on the rom console */

#if 1
static char romprint_hextable[] = "0123456789abcdef@";
#define ROM_PUTX(v) \
  do { \
    (*MONRELOC(putcptr,MG_putc)) \
			 ((romprint_hextable+NEXT_RAMBASE)[((v)>>4)&0xf]); \
    (*MONRELOC(putcptr,MG_putc)) \
			 ((romprint_hextable+NEXT_RAMBASE)[(v)&0xf]); \
	} while(0);
#else
#define lookup_hex(v)  ((v)>9?('a'+(v)-0xa):('0'+(v)))
#define ROM_PUTX(v) \
  do { \
    (*MONRELOC(putcptr,MG_putc)) \
			 (lookup_hex(((v)>>4)&0xf)); \
    (*MONRELOC(putcptr,MG_putc)) \
			 (lookup_hex((v)&0xf)); \
	} while(0);
#endif
#endif

u_char rom_enetaddr[6];
u_char rom_boot_dev[20];
u_char rom_boot_arg[20];
u_char rom_boot_info[20];
u_char rom_boot_file[20];
u_char rom_bootfile[MG_boot_how-MG_bootfile];
char rom_machine_type;

u_char *rom_return_sp;
u_int rom_mon_stack;
u_char rom_image[0x2000];
vm_offset_t rom_image_base;
u_int rom_vbr;;

paddr_t rom_reboot_vect;

void
next68k_bootargs(args)
     unsigned char *args[];
{
#ifdef DDB
	int i;
	Elf_Ehdr *ehdr;
	Elf_Shdr *shp;
	vaddr_t minsym, maxsym;
	char *reloc_end, *reloc_elfmag;
#endif

	RELOC(rom_return_sp,u_char *) = args[0];
	RELOC(mg,char *) = args[1];

	ROM_PUTS("Welcome to NetBSD/next68k\r\n");

#ifdef DDB

	/*
	 * Check the ELF headers.
	 */

	reloc_end = end + NEXT_RAMBASE;
	reloc_elfmag = ELFMAG + NEXT_RAMBASE;
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
	ROM_PUTX(MONRELOC(char,MG_machine_type));
	ROM_PUTS("\r\nboard rev = 0x");
	ROM_PUTX(MONRELOC(char,MG_board_rev));
	ROM_PUTS("\r\ndmachip = 0x");
	ROM_PUTX(MONRELOC(int,MG_dmachip)>>24&0xff);
	ROM_PUTX(MONRELOC(int,MG_dmachip)>>16&0xff);
	ROM_PUTX(MONRELOC(int,MG_dmachip)>>8&0xff);
	ROM_PUTX(MONRELOC(int,MG_dmachip)>>0&0xff);
	ROM_PUTS("\r\ndiskchip = 0x");
	ROM_PUTX(MONRELOC(int,MG_diskchip)>>24&0xff);
	ROM_PUTX(MONRELOC(int,MG_diskchip)>>16&0xff);
	ROM_PUTX(MONRELOC(int,MG_diskchip)>>8&0xff);
	ROM_PUTX(MONRELOC(int,MG_diskchip)>>0&0xff);
	ROM_PUTS("\r\n");


  /* Construct the segment list */
  {        
		u_int msize16;
		u_int msize4;
		u_int msize1;
    int i;
    int j = 0;

		if (MONRELOC(char,MG_machine_type) == NeXT_X15) {
			msize16 = 0x1000000;
			msize4  = 0x400000;
			msize1  = 0x100000;
			ROM_PUTS("Looks like a NeXT_X15\r\n");
		} else if (MONRELOC(char,MG_machine_type) == NeXT_WARP9C) {
			msize16 = 0x800000;
			msize4  = 0x200000;
			msize1  = 0x80000;				/* ? */
			ROM_PUTS("Looks like a NeXT_WARP9C\r\n");
		} else if (MONRELOC(char,MG_machine_type) == NeXT_WARP9) {
			msize16 = 0x1000000;
			msize4  = 0x400000;
			msize1  = 0x100000;
			ROM_PUTS("Looks like a NeXT_WARP9\r\n");
		} else if (MONRELOC(char,MG_machine_type) == NeXT_TURBO_COLOR) {
			msize16 = 0x2000000;
			msize4  = 0x800000;
			msize1  = 0x200000;
			ROM_PUTS("Looks like a NeXT_TURBO_COLOR\r\n");
		} else {
			msize16 = 0x100000;
			msize4  = 0x100000;
			msize1  = 0x100000;
			ROM_PUTS("Unrecognized machine_type\r\n");
		}

		RELOC(rom_machine_type, char) = MONRELOC(char, MG_machine_type);

    for (i=0;i<N_SIMM;i++) {
			
			ROM_PUTS("Memory bank 0x");
			ROM_PUTX(i);
			ROM_PUTS(" has value 0x");
			ROM_PUTX(MONRELOC(char,MG_simm+i))
			ROM_PUTS("\r\n");
			
      if ((MONRELOC(char,MG_simm+i) & SIMM_SIZE) != SIMM_SIZE_EMPTY) {
        RELOC(phys_seg_list[j].ps_start, vm_offset_t) 
          = NEXT_RAMBASE+(i*msize16);
      }
      if ((MONRELOC(char,MG_simm+i) & SIMM_SIZE) == SIMM_SIZE_16MB) {
        RELOC(phys_seg_list[j].ps_end, vm_offset_t) = 
          RELOC(phys_seg_list[j].ps_start, vm_offset_t) +
						msize16;
        j++;
      } 
      if ((MONRELOC(char,MG_simm+i) & SIMM_SIZE) == SIMM_SIZE_4MB) {
        RELOC(phys_seg_list[j].ps_end, vm_offset_t) = 
          RELOC(phys_seg_list[j].ps_start, vm_offset_t) +
						msize4;
        j++;
      }
      if ((MONRELOC(char,MG_simm+i) & SIMM_SIZE) == SIMM_SIZE_1MB) {
        RELOC(phys_seg_list[j].ps_end, vm_offset_t) = 
          RELOC(phys_seg_list[j].ps_start, vm_offset_t) +
						msize1;
        j++;
      }
    }

		/* The NeXT ROM or something appears to reserve the very
		 * top of memory
		 */
		RELOC(phys_seg_list[j-1].ps_end, vm_offset_t) -= 0x2000;
		RELOC(rom_image_base, vm_offset_t) = RELOC(phys_seg_list[j-1].ps_end, vm_offset_t);

    /* pmap is unhappy if it is not null terminated */
    for(;j<MAX_PHYS_SEGS;j++) {
      RELOC(phys_seg_list[j].ps_start, vm_offset_t) = 0;
      RELOC(phys_seg_list[j].ps_end, vm_offset_t) = 0;
    }
  }

	{
		int i;
		ROM_PUTS("Memory segments found:\r\n");
		for (i=0;RELOC(phys_seg_list[i].ps_start, vm_offset_t);i++) {
			ROM_PUTS("\t0x");
			ROM_PUTX((RELOC(phys_seg_list[i].ps_start, vm_offset_t)>>24)&0xff);
			ROM_PUTX((RELOC(phys_seg_list[i].ps_start, vm_offset_t)>>16)&0xff);
			ROM_PUTX((RELOC(phys_seg_list[i].ps_start, vm_offset_t)>>8)&0xff);
			ROM_PUTX((RELOC(phys_seg_list[i].ps_start, vm_offset_t)>>0)&0xff);
			ROM_PUTS(" - 0x");
			ROM_PUTX((RELOC(phys_seg_list[i].ps_end, vm_offset_t)>>24)&0xff);
			ROM_PUTX((RELOC(phys_seg_list[i].ps_end, vm_offset_t)>>16)&0xff);
			ROM_PUTX((RELOC(phys_seg_list[i].ps_end, vm_offset_t)>>8)&0xff);
			ROM_PUTX((RELOC(phys_seg_list[i].ps_end, vm_offset_t)>>0)&0xff);
			ROM_PUTS("\r\n");
		}
	}

  /* Read the ethernet address from rom, this should be done later
   * in device driver somehow.
   */
  {
    int i;
		ROM_PUTS("Ethernet address: ");
    for(i=0;i<6;i++) {
      RELOC(rom_enetaddr[i], u_char) = MONRELOC(u_char *, MG_clientetheraddr)[i];
			ROM_PUTX(RELOC(rom_enetaddr[i],u_char));
			if (i < 5) ROM_PUTS(":");
    }
		ROM_PUTS("\r\n");
  }

	/* Read the boot args
	 */
	{
		int i;
		for(i=0;i<sizeof(rom_bootfile);i++) {
			RELOC(rom_bootfile[i], u_char) = MONRELOC(u_char, MG_bootfile+i);
		}

		for(i=0;i<sizeof(rom_boot_dev);i++) {
			RELOC(rom_boot_dev[i], u_char) = MONRELOC(u_char *, MG_boot_dev)[i];
			if (MONRELOC(u_char *, MG_boot_dev)[i] == '\0') break;
		}
		RELOC(rom_boot_dev[sizeof(rom_boot_dev)-1], u_char) = 0;

		for(i=0;i<sizeof(rom_boot_arg);i++) {
			RELOC(rom_boot_arg[i], u_char) = MONRELOC(u_char *, MG_boot_arg)[i];
			if (MONRELOC(u_char *, MG_boot_arg)[i] == '\0') break;
		}
		RELOC(rom_boot_arg[sizeof(rom_boot_arg)-1], u_char) = 0;

		for(i=0;i<sizeof(rom_boot_info);i++) {
			RELOC(rom_boot_info[i], u_char) = MONRELOC(u_char *, MG_boot_info)[i];
			if (MONRELOC(u_char *, MG_boot_info)[i] == '\0') break;
		}
		RELOC(rom_boot_info[sizeof(rom_boot_info)-1], u_char) = 0;

		for(i=0;i<sizeof(rom_boot_file);i++) {
			RELOC(rom_boot_file[i], u_char) = MONRELOC(u_char *, MG_boot_file)[i];
			if (MONRELOC(u_char *, MG_boot_file)[i] == '\0') break;
		}
		RELOC(rom_boot_file[sizeof(rom_boot_file)-1], u_char) = 0;

		RELOC(rom_mon_stack, u_int) = MONRELOC(u_int, MG_mon_stack);
		RELOC(rom_vbr, u_int) = MONRELOC(u_int, MG_vbr);
		RELOC(rom_reboot_vect, paddr_t) = MONRELOC(paddr_t *, MG_vbr)[45]; /* trap #13 */

		for(i=0;i<sizeof(rom_image);i++) {
			RELOC(rom_image[i], u_char) = *(u_char *)(RELOC(rom_image_base, vm_offset_t) + i);
		}
	}

	ROM_PUTS("Check serial port A for console.\r\n");
}
