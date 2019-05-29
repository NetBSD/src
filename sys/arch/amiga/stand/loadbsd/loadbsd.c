/*	$NetBSD: loadbsd.c,v 1.36 2019/05/29 02:34:18 msaitoh Exp $	*/

/*
 * Copyright (c) 1994 Michael L. Hitch
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <err.h>

#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <graphics/gfxbase.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <libraries/configregs.h>
#include <libraries/configvars.h>
#include <proto/expansion.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* Get definitions for boothowto */
#include "sys/reboot.h"
#include "inttypes.h"
#include "loadfile.h"

#undef AOUT_LDPGSZ
#define AOUT_LDPGSZ 8192

#undef sleep
#define sleep(n) if (!t_flag) (void)Delay(50*n)

/*
 *	Version history:
 *	1.x	Kernel startup interface version check.
 *	2.0	Added symbol table end address and symbol table support.
 *	2.1	03/23/94 - Round up end of fastram segment.
 *		Check fastram segment size for minimum of 2M.
 *		Use largest segment of highest priority if -p option.
 *		Print out fastram size in KB if not a multiple of MB.
 *	2.2	03/24/94 - Zero out all unused registers.
 *		Started version history comment.
 *	2.3	04/26/94 - Added -D option to enter debugger on boot.
 *	2.4	04/30/94 - Cpuid includes base machine type.
 *		Also check if CPU is capable of running NetBSD.
 *	2.5	05/17/94 - Add check for "A3000 bonus".
 *	2.6	06/05/94 - Added -c option to override machine type.
 *	2.7	06/15/94 - Pass E clock frequency.
 *	2.8	06/22/94 - Fix supervisor stack usage.
 *	2.9	06/26/94 - Use PAL flag for E clock freq on pre 2.0 WB
 *		Added AGA enable parameter
 *	2.10	12/22/94 - Use FindResident() & OpenResource() for machine
 *		type detection.
 *		Add -n flag & option for non-contiguous memory.
 *		01/28/95 - Corrected -n on usage & help messages.
 *	2.11	03/12/95 - Check kernel size against chip memory size.
 *	2.12	11/11/95 - Add -I option to inhibit synchronous transfer
 *		11/12/95 - New kernel startup interface version - to
 *		support loading kernel image to fastmem rather than chipmem.
 *	2.13	04/15/96 - Direct load to fastmem.
 *		Add -Z flag to force chipmem load.
 *		Moved test mode exit to later - kernel image is created
 *		and startup interface version checked in test mode.
 *		Add -s flag for compatibility to bootblock loader.
 *		05/02/96 - Add a maximum startup interface version level
 *		to allow future kernel compatibility.
 *	2.14	06/26/96 is - Add first version of kludges needed to
 *		boot on DraCos. This can probably be done a bit more cleanly
 *		using TTRs, but it works for now.
 *	2.15	07/28/96 is - Add first version of kludges needed to
 *		get FusionForty kickrom'd memory back. Hope this doesn't
 *		break anything else.
 *	2.16	07/08/00 - Added bootverbose support.
 *		01/15/03 - Plugged resource leaks.
 *		Fixed printf() statements.
 *		Ansified.
 *	3.0	01/16/03 - ELF support through loadfile() interface.
 */
static const char _version[] = "$VER: LoadBSD 3.0 (16.1.2003)";

/*
 * Kernel startup interface version
 *	1:	first version of loadbsd
 *	2:	needs esym location passed in a4
 *	3:	load kernel image into fastmem rather than chipmem
 *	MAX:	highest version with backward compatibility.
 */
#define KERNEL_STARTUP_VERSION		3
#define	KERNEL_STARTUP_VERSION_MAX	9

#define DRACOREVISION (*(UBYTE *)0x02000009)
#define DRACOMMUMARGIN 0x200000

#define MAXMEMSEG	16
struct boot_memlist {
	u_int	m_nseg; /* num_mem; */
	struct boot_memseg {
		u_int	ms_start;
		u_int	ms_size;
		u_short	ms_attrib;
		short	ms_pri;
	} m_seg[MAXMEMSEG];
};
struct boot_memlist memlist;
struct boot_memlist *kmemlist;

void get_mem_config (void **, u_long *, u_long *);
void get_cpuid (void);
void get_eclock (void);
void get_AGA (void);
void usage (void);
void verbose_usage (void);
void startit (void *, u_long, u_long, void *, u_long, u_long, int, void *,
		int, int, u_long, u_long, int);
extern u_long startit_sz;

extern char *optarg;
extern int optind;

struct ExpansionBase *ExpansionBase = NULL;
struct GfxBase *GfxBase = NULL;

int k_flag;
int p_flag;
int t_flag;
int reqmemsz;
int S_flag;
u_long I_flag;
int Z_flag;
u_long cpuid;
long eclock_freq;
long amiga_flags;
char *program_name;
u_char *kp;
u_long kpsz;

void
exit_func(void)
{
	if (kp)
		FreeMem(kp, kpsz);
	if (ExpansionBase)
		CloseLibrary((struct Library *)ExpansionBase);
	if (GfxBase)
		CloseLibrary((struct Library *)GfxBase);
}

int
main(int argc, char **argv)
{
	struct ConfigDev *cd, *kcd;
	u_long fmemsz, cmemsz, ksize, marks[MARK_MAX];
	int boothowto, ncd, i, mem_ix, ch;
	u_short kvers;
	int *nkcd;
	void *fmem;
	char *esym;
	void (*start_it) (void *, u_long, u_long, void *, u_long, u_long,
	     int, void *, int, int, u_long, u_long, int) = startit;
	char *kernel_name;

	atexit(exit_func);

	program_name = argv[0];
	boothowto = RB_SINGLE;

	if (argc < 2)
		usage();

	if ((GfxBase = (void *)OpenLibrary(GRAPHICSNAME, 0)) == NULL)
		err(20, "can't open graphics library");
	if ((ExpansionBase=(void *)OpenLibrary(EXPANSIONNAME, 0)) == NULL)
		err(20, "can't open expansion library");

	while ((ch = getopt(argc, argv, "aAbCc:DhI:km:n:qptsSvVZ")) != -1) {
		switch (ch) {
		case 'k':
			k_flag = 1;
			break;
		case 'a':
			boothowto &= ~(RB_SINGLE);
			boothowto |= RB_AUTOBOOT;
			break;
		case 'b':
			boothowto |= RB_ASKNAME;
			break;
		case 'p':
			p_flag = 1;
			break;
		case 't':
			t_flag = 1;
			break;
		case 'm':
			reqmemsz = atoi(optarg) * 1024;
			break;
		case 's':
			boothowto &= ~(RB_AUTOBOOT);
			boothowto |= RB_SINGLE;
			break;
		case 'q':
			boothowto |= AB_QUIET;
			break;
		case 'v':
			boothowto |= AB_VERBOSE;
			break;
		case 'V':
			fprintf(stderr,"%s\n",_version + 6);
			break;
		case 'S':
			S_flag = 1;
			break;
		case 'D':
			boothowto |= RB_KDB;
			break;
		case 'c':
			cpuid = atoi(optarg) << 16;
			break;
		case 'A':
			amiga_flags |= 1;
			break;
		case 'n':
			i = atoi(optarg);
			if (i >= 0 && i <= 3)
				amiga_flags |= i << 1;
			else
				err(20, "-n option must be 0, 1, 2, or 3");
			break;
		case 'C':
			amiga_flags |= (1 << 3);
			break;
		case 'I':
			I_flag = strtoul(optarg, NULL, 16);
			break;
		case 'Z':
			Z_flag = 1;
			break;
		case 'h':
			verbose_usage();
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	kernel_name = argv[0];

	for (cd = 0, ncd = 0; cd = FindConfigDev(cd, -1, -1); ncd++)
		;
	get_cpuid();
	get_mem_config(&fmem, &fmemsz, &cmemsz);
	get_eclock();
	get_AGA();

/*
 * XXX Call loadfile with COUNT* options to get size
 * XXX Allocate memory for kernel + additional data
 * XXX Call loadfile with LOAD* options to load text/data/symbols
 */
	marks[MARK_START] = 0;
	if (loadfile(kernel_name, marks,
	    COUNT_TEXT|COUNT_TEXTA|COUNT_DATA|COUNT_BSS|
	    (S_flag ? (COUNT_SYM|COUNT_HDR) : 0)) == -1) {
		err(20, "unable to parse kernel image");
	}
	ksize = ((marks[MARK_END] + 3) & ~3)
	    + sizeof(*nkcd) + ncd * sizeof(*cd)
	    + sizeof(*nkcd) + memlist.m_nseg * sizeof(struct boot_memseg);

	if (t_flag) {
		for (i = 0; i < memlist.m_nseg; ++i) {
			printf("mem segment %d: start=%08lx size=%08lx"
			    " attribute=%04lx pri=%d\n",
			    i + 1,
			    memlist.m_seg[i].ms_start,
			    memlist.m_seg[i].ms_size,
			    memlist.m_seg[i].ms_attrib,
			    memlist.m_seg[i].ms_pri);
		}
		printf("kernel size: %ld\n", ksize);
	}

	kpsz = ksize + 256 + startit_sz;
	kp = (u_char *)AllocMem(kpsz, MEMF_FAST|MEMF_REVERSE);
	if (kp == NULL)
		err(20, "failed alloc %d", ksize);

	marks[MARK_START] = (u_long)kp;
	if (loadfile(kernel_name, marks,
	    LOAD_TEXT|LOAD_TEXTA|LOAD_DATA|LOAD_BSS|
	    (S_flag ? (LOAD_SYM|LOAD_HDR) : 0)) == -1) {
		err(20, "unable to load kernel image");
	}
	marks[MARK_END] = (marks[MARK_END] + 3) & ~3;

	if (k_flag) {
		fmem += 4 * 1024 * 1024;
		fmemsz -= 4 * 1024 * 1024;
	}
	if (reqmemsz && reqmemsz <= fmemsz)
		fmemsz = reqmemsz;

	if (boothowto & RB_AUTOBOOT)
		printf("Autobooting...");
	if (boothowto & RB_ASKNAME)
		printf("Askboot...");

	printf("Using %ld%c FASTMEM at 0x%lx, %ldM CHIPMEM\n",
	    (fmemsz & 0xfffff) ? fmemsz >> 10 : fmemsz >> 20,
	    (fmemsz & 0xfffff) ? 'K' : 'M', (u_long)fmem, cmemsz >> 20);

	kvers = *(u_short *)(marks[MARK_ENTRY] - 2);
	if (kvers == 0x4e73) kvers = 0;
	if (kvers > KERNEL_STARTUP_VERSION_MAX)
		err(20, "newer loadbsd required: %d\n", kvers);
	if (kvers > KERNEL_STARTUP_VERSION) {
		printf("****************************************************\n"
		       "*** Notice:  this kernel has features which require\n"
		       "*** a newer version of loadbsd.  To allow the use of\n"
		       "*** any newer features or capabilities, you should\n"
		       "*** update to a newer version of loadbsd\n"
		       "****************************************************\n");
		sleep(3);	/* even more time to see that message */
	}

	/*
	 * give them a chance to read the information...
	 */
	sleep(2);

	nkcd = (int *)marks[MARK_END];
	esym = 0;
	/*
	 * If symbols loaded and kernel can handle them, set esym to end.
	 */
	if (marks[MARK_SYM] != marks[MARK_START]) {
		if (kvers > 1)  {
			esym = (void *)(marks[MARK_END] - marks[MARK_START]);
		}
		else {
			/*
			 * suppress symbols
			 */
			nkcd = (int *)marks[MARK_SYM];
		}
	}

	*nkcd = ncd;
	kcd = (struct ConfigDev *)(nkcd + 1);
	while((cd = FindConfigDev(cd, -1, -1))) {
		memcpy(kcd, cd, sizeof(*kcd));
		if (((cpuid >> 24) == 0x7d) &&
		    ((u_long)kcd->cd_BoardAddr < 0x1000000)) {
			if (t_flag)
				printf("Transformed Z2 device from %08lx ", (u_long)kcd->cd_BoardAddr);
			kcd->cd_BoardAddr += 0x3000000;
			if (t_flag)
				printf("to %08lx\n", (u_long)kcd->cd_BoardAddr);
		}
		++kcd;
	}

	kmemlist = (struct boot_memlist *)kcd;
	kmemlist->m_nseg = memlist.m_nseg;
	for (mem_ix = 0; mem_ix < memlist.m_nseg; mem_ix++)
		kmemlist->m_seg[mem_ix] = memlist.m_seg[mem_ix];

	if (kvers > 2 && Z_flag == 0) {
		/*
		 * Kernel supports direct load to fastmem, and the -Z
		 * option was not specified.  Copy startup code to end
		 * of kernel image and set start_it.
		 */
		if ((void *)kp < fmem) {
			printf("Kernel at %08lx, Fastmem used at %08lx\n",
			    (u_long)kp, (u_long)fmem);
			err(20, "Can't copy upwards yet.\nDefragment your memory and try again OR try the -p OR try the -Z options.");
		}
		start_it = (void (*)())kp + ksize + 256;
		memcpy(start_it, startit, startit_sz);
		CacheClearU();
		printf("*** Loading from %08lx to Fastmem %08lx ***\n",
		    (u_long)kp, (u_long)fmem);
		sleep(2);
	} else {
		/*
		 * Either the kernel doesn't support loading directly to
		 * fastmem or the -Z flag was given.  Verify kernel image
		 * fits into chipmem.
		 */
		if (ksize >= cmemsz) {
			printf("Kernel size %ld exceeds Chip Memory of %ld\n",
			    ksize, cmemsz);
			err(20, "Insufficient Chip Memory for kernel");
		}
		Z_flag = 1;
		printf("*** Loading from %08lx to Chipmem ***\n", (u_long)kp);
	}

	/*
	 * if test option set, done
	 */
	if (t_flag) {
		exit(0);
	}

	/*
	 * XXX AGA startup - may need more
	 */
	LoadView(NULL);		/* Don't do this if AGA active? */
	start_it(kp, ksize, marks[MARK_ENTRY] - marks[MARK_START], fmem, fmemsz, cmemsz,
	    boothowto, esym, cpuid, eclock_freq, amiga_flags, I_flag, Z_flag == 0);
	/*NOTREACHED*/
}

void
get_mem_config(void **fmem, u_long *fmemsz, u_long *cmemsz)
{
	struct MemHeader *mh, *nmh;
	u_int nmem, eseg, segsz, seg, nseg, nsegsz;
	char mempri;

	nmem = 0;
	mempri = -128;
	*fmemsz = 0;
	*cmemsz = 0;

	/*
	 * walk thru the exec memory list
	 */
	Forbid();
	for (mh  = (void *) SysBase->MemList.lh_Head;
	    nmh = (void *) mh->mh_Node.ln_Succ; mh = nmh) {

		nseg = (u_int)mh->mh_Lower;
		nsegsz = (u_int)mh->mh_Upper - nseg;

		segsz = nsegsz;
		seg = (u_int)CachePreDMA((APTR)nseg, (LONG *)&segsz, 0L);
		nsegsz -= segsz, nseg += segsz;
		for (;segsz;
		    segsz = nsegsz,
		    seg = (u_int)CachePreDMA((APTR)nseg, (LONG *)&segsz, DMA_Continue),
		    nsegsz -= segsz, nseg += segsz, ++nmem) {

			if (t_flag)
				printf("Translated %08x sz %08x to %08x sz %08x\n",
				    nseg - segsz, nsegsz + segsz, seg, segsz);

			eseg = seg + segsz;

			if ((cpuid >> 24) == 0x7D) {
				/* DraCo MMU table kludge */

				segsz = ((segsz -1) | 0xfffff) + 1;
				seg = eseg - segsz;

				/*
				 * Only use first SIMM to boot; we know it is VA==PA.
				 * Enter into table and continue. Yes,
				 * this is ugly.
				 */
				if (seg != 0x40000000) {
					memlist.m_seg[nmem].ms_attrib = mh->mh_Attributes;
					memlist.m_seg[nmem].ms_pri = mh->mh_Node.ln_Pri;
					memlist.m_seg[nmem].ms_size = segsz;
					memlist.m_seg[nmem].ms_start = seg;
					++nmem;
					continue;
				}

				memlist.m_seg[nmem].ms_attrib = mh->mh_Attributes;
				memlist.m_seg[nmem].ms_pri = mh->mh_Node.ln_Pri;
				memlist.m_seg[nmem].ms_size = DRACOMMUMARGIN;
				memlist.m_seg[nmem].ms_start = seg;

				++nmem;
				seg += DRACOMMUMARGIN;
				segsz -= DRACOMMUMARGIN;
			}

			memlist.m_seg[nmem].ms_attrib = mh->mh_Attributes;
			memlist.m_seg[nmem].ms_pri = mh->mh_Node.ln_Pri;
			memlist.m_seg[nmem].ms_size = segsz;
			memlist.m_seg[nmem].ms_start = seg;

			if ((mh->mh_Attributes & (MEMF_CHIP|MEMF_FAST)) == MEMF_CHIP) {
				/*
				 * there should hardly be more than one entry for
				 * chip mem, but handle it the same nevertheless
				 * cmem always starts at 0, so include vector area
				 */
				memlist.m_seg[nmem].ms_start = seg = 0;
				/*
				 * round to multiple of 512K
				 */
				segsz = (segsz + 512 * 1024 - 1) & -(512 * 1024);
				memlist.m_seg[nmem].ms_size = segsz;
				if (segsz > *cmemsz)
					*cmemsz = segsz;
				continue;
			}
			/*
			 * some heuristics..
			 */
			seg &= -AOUT_LDPGSZ;
			eseg = (eseg + AOUT_LDPGSZ - 1) & -AOUT_LDPGSZ;

			/*
			 * get the mem back stolen by incore kickstart on
			 * A3000 with V36 bootrom.
			 */
			if (eseg == 0x07f80000)
				eseg = 0x08000000;

			/*
			 * or by zkick on a A2000.
			 */
			if (seg == 0x280000 &&
			    strcmp(mh->mh_Node.ln_Name, "zkick memory") == 0)
				seg = 0x200000;
			/*
			 * or by Fusion Forty fastrom
			 */
			if ((seg & ~(1024*1024-1)) == 0x11000000) {
				/*
				 * XXX we should test the name.
				 * Unfortunately, the memory is just called
				 * "32 bit memory" which isn't very specific.
				 */
				seg = 0x11000000;
			}

			segsz = eseg - seg;
			memlist.m_seg[nmem].ms_start = seg;
			memlist.m_seg[nmem].ms_size = segsz;
			/*
			 *  If this segment is smaller than 2M,
			 *  don't use it to load the kernel
			 */
			if (segsz < 2 * 1024 * 1024)
				continue;
			/*
			 * if p_flag is set, select memory by priority
			 * instead of size
			 */
			if ((!p_flag && segsz > *fmemsz) || (p_flag &&
			   mempri <= mh->mh_Node.ln_Pri && segsz > *fmemsz)) {
				*fmemsz = segsz;
				*fmem = (void *)seg;
				mempri = mh->mh_Node.ln_Pri;
			}

		}
	}
	memlist.m_nseg = nmem;
	Permit();
}

/*
 * Try to determine the machine ID by searching the resident module list
 * for modules only present on specific machines.  (Thanks, Bill!)
 */
void
get_cpuid(void)
{
	cpuid |= SysBase->AttnFlags;	/* get FPU and CPU flags */
	if ((cpuid & AFB_68020) == 0)
		err(20, "CPU not supported");
	if (cpuid & 0xffff0000) {
		if ((cpuid >> 24) == 0x7D)
			return;

		switch (cpuid >> 16) {
		case 500:
		case 600:
		case 1000:
		case 1200:
		case 2000:
		case 3000:
		case 4000:
			return;
		default:
			printf("machine Amiga %ld is not recognized\n",
			    cpuid >> 16);
			exit(1);
		}
	}
	if (FindResident("A4000 Bonus") || FindResident("A4000 bonus")
	    || FindResident("A1000 Bonus"))
		cpuid |= 4000 << 16;
	else if (FindResident("A3000 Bonus") || FindResident("A3000 bonus"))
		cpuid |= 3000 << 16;
	else if (OpenResource("card.resource")) {
		/* Test for AGA? */
		cpuid |= 1200 << 16;
	} else if (OpenResource("draco.resource")) {
		cpuid |= (32000 | DRACOREVISION) << 16;
	}
	/*
	 * Nothing found, it's probably an A2000 or A500
	 */
	if ((cpuid >> 16) == 0)
		cpuid |= 2000 << 16;
}

void
get_eclock(void)
{
	/* Fix for 1.3 startups? */
	if (SysBase->LibNode.lib_Version > 36)
		eclock_freq = SysBase->ex_EClockFrequency;
	else
		eclock_freq = (GfxBase->DisplayFlags & PAL) ?
		    709379 : 715909;
}

void
get_AGA(void)
{
	/*
	 * Determine if an AGA mode is active
	 */
}

__asm("
	.text

_startit:
	movel	sp,a3
	movel	4:w,a6
	lea	pc@(start_super),a5
	jmp	a6@(-0x1e)		| supervisor-call

start_super:
	movew	#0x2700,sr

	| the BSD kernel wants values into the following registers:
	| a0:  fastmem-start
	| d0:  fastmem-size
	| d1:  chipmem-size
	| d3:  Amiga specific flags
	| d4:  E clock frequency
	| d5:  AttnFlags (cpuid)
	| d7:  boothowto
	| a4:  esym location
	| a2:  Inhibit sync flags
	| All other registers zeroed for possible future requirements.

	lea	pc@(_startit),sp	| make sure we have a good stack ***

	movel	a3@(4),a1		| loaded kernel
	movel	a3@(8),d2		| length of loaded kernel
|	movel	a3@(12),sp		| entry point in stack pointer
	movel	a3@(12),a6		| push entry point		***
	movel	a3@(16),a0		| fastmem-start
	movel	a3@(20),d0		| fastmem-size
	movel	a3@(24),d1		| chipmem-size
	movel	a3@(28),d7		| boothowto
	movel	a3@(32),a4		| esym
	movel	a3@(36),d5		| cpuid
	movel	a3@(40),d4		| E clock frequency
	movel	a3@(44),d3		| Amiga flags
	movel	a3@(48),a2		| Inhibit sync flags
	movel	a3@(52),d6		| Load to fastmem flag
	subl	a5,a5			| target, load to 0

	cmpb	#0x7D,a3@(36)		| is it DraCo?
	beq	nott			| yes, switch off MMU later

					| no, it is an Amiga:

|	movew	#0xf00,0xdff180		|red
|	moveb	#0,0x200003c8
|	moveb	#63,0x200003c9
|	moveb	#0,0x200003c9
|	moveb	#0,0x200003c9

	movew	#(1<<9),0xdff096	| disable DMA on Amigas.

| ------ mmu off start -----

	btst	#3,d5			| AFB_68040,SysBase->AttnFlags
	beq	not040

| Turn off 68040/060 MMU

	subl	a3,a3
	.word 0x4e7b,0xb003		| movec a3,tc
	.word 0x4e7b,0xb806		| movec a3,urp
	.word 0x4e7b,0xb807		| movec a3,srp
	.word 0x4e7b,0xb004		| movec a3,itt0
	.word 0x4e7b,0xb005		| movec a3,itt1
	.word 0x4e7b,0xb006		| movec a3,dtt0
	.word 0x4e7b,0xb007		| movec a3,dtt1
	bra	nott

not040:
	lea	pc@(zero),a3
	pmove	a3@,tc			| Turn off MMU
	lea	pc@(nullrp),a3
	pmove	a3@,crp			| Turn off MMU some more
	pmove	a3@,srp			| Really, really, turn off MMU

| Turn off 68030 TT registers

	btst	#2,d5			| AFB_68030,SysBase->AttnFlags
	beq	nott			| Skip TT registers if not 68030
	lea	pc@(zero),a3
	.word 0xf013,0x0800		| pmove a3@,tt0 (gas only knows about 68851 ops..)
	.word 0xf013,0x0c00		| pmove a3@,tt1 (gas only knows about 68851 ops..)

nott:
| ---- mmu off end ----
|	movew	#0xf60,0xdff180		| orange
|	moveb	#0,0x200003c8
|	moveb	#63,0x200003c9
|	moveb	#24,0x200003c9
|	moveb	#0,0x200003c9

| ---- copy kernel start ----

	tstl	d6			| Can we load to fastmem?
	beq	L0			| No, leave destination at 0
	movl	a0,a5			| Move to start of fastmem chunk
	addl	a0,a6			| relocate kernel entry point
L0:
	movl	a1@+,a5@+
	subl	#4,d2
	bcc	L0

	lea	pc@(ckend),a1
	movl	a5,sp@-
	movl	#_startit_end - ckend,d2
L2:
	movl	a1@+,a5@+
	subl	#4,d2
	bcc	L2

	btst	#3,d5
	jeq	L1
	.word	0xf4f8
L1:
	movql	#0,d2			| switch off cache to ensure we use
	movec	d2,cacr			| valid kernel data

|	movew	#0xFF0,0xdff180		| yellow
|	moveb	#0,0x200003c8
|	moveb	#63,0x200003c9
|	moveb	#0,0x200003c9
|	moveb	#0,0x200003c9
	rts

| ---- copy kernel end ----

ckend:
|	movew	#0x0ff,0xdff180		| petrol
|	moveb	#0,0x200003c8
|	moveb	#0,0x200003c9
|	moveb	#63,0x200003c9
|	moveb	#63,0x200003c9

	movl	d5,d2
	roll	#8,d2
	cmpb	#0x7D,d2
	jne	noDraCo

| DraCo: switch off MMU now:

	subl	a3,a3
	.word 0x4e7b,0xb003		| movec a3,tc
	.word 0x4e7b,0xb806		| movec a3,urp
	.word 0x4e7b,0xb807		| movec a3,srp
	.word 0x4e7b,0xb004		| movec a3,itt0
	.word 0x4e7b,0xb005		| movec a3,itt1
	.word 0x4e7b,0xb006		| movec a3,dtt0
	.word 0x4e7b,0xb007		| movec a3,dtt1

noDraCo:
	moveq	#0,d2			| zero out unused registers
	moveq	#0,d6			| (might make future compatibility
	movel	d6,a1			|  would have known contents)
	movel	d6,a3
	movel	d6,a5
	movel	a6,sp			| entry point into stack pointer
	movel	d6,a6

|	movew	#0x0F0,0xdff180		| green
|	moveb	#0,0x200003c8
|	moveb	#0,0x200003c9
|	moveb	#63,0x200003c9
|	moveb	#0,0x200003c9

	jmp	sp@			| jump to kernel entry point

| A do-nothing MMU root pointer (includes the following long as well)

nullrp:	.long	0x7fff0001
zero:	.long	0

_startit_end:

	.data
_startit_sz: .long _startit_end-_startit

	.text
");

void
usage(void)
{
	fprintf(stderr, "usage: %s [-abhkpstACDSVZ] [-c machine] [-m mem] [-n mode] [-I sync-inhibit] kernel\n",
	    program_name);
	exit(1);
}

void
verbose_usage(void)
{
	fprintf(stderr, "
NAME
\t%s - loads NetBSD from amiga dos.
SYNOPSIS
\t%s [-abhkpstADSVZ] [-c machine] [-m mem] [-n flags] [-I sync-inhibit] kernel
OPTIONS
\t-a  Boot up to multiuser mode.
\t-A  Use AGA display mode, if available.
\t-b  Ask for which root device.
\t    Its possible to have multiple roots and choose between them.
\t-c  Set machine type. [e.g 3000; use 32000+N for DraCo rev. N]
\t-C  Use Serial Console.
\t-D  Enter debugger
\t-h  This help message.
\t-I  Inhibit sync negotiation. Option value is bit-encoded targets.
\t-k  Reserve the first 4M of fast mem [Some one else
\t    is going to have to answer what that it is used for].
\t-m  Tweak amount of available memory, for finding minimum amount
\t    of memory required to run. Sets fastmem size to specified
\t    size in Kbytes.
\t-n  Enable multiple non-contiguous memory: value = 0 (disabled),
\t    1 (two segments), 2 (all avail segments), 3 (same as 2?).
\t-p  Use highest priority fastmem segement instead of the largest
\t    segment. The higher priority segment is usually faster
\t    (i.e. 32 bit memory), but some people have smaller amounts
\t    of 32 bit memory.
\t-q  Boot up in quiet mode.
\t-s  Boot up in singleuser mode (default).
\t-S  Include kernel symbol table.
\t-t  This is a *test* option.  It prints out the memory
\t    list information being passed to the kernel and also
\t    exits without actually starting NetBSD.
\t-v  Boot up in verbose mode.
\t-V  Version of loadbsd program.
\t-Z  Force kernel load to chipmem.
HISTORY
\tThis version supports Kernel version 720 +\n",
      program_name, program_name);
      exit(1);
}

static void
_Vdomessage(int doerrno, const char *fmt, va_list args)
{
	fprintf(stderr, "%s: ", program_name);
	if (fmt) {
		vfprintf(stderr, fmt, args);
		fprintf(stderr, ": ");
	}
	if (doerrno && errno < sys_nerr) {
		fprintf(stderr, "%s", strerror(errno));
	}
	fprintf(stderr, "\n");
}

void
err(int eval, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	_Vdomessage(1, fmt, ap);
	va_end(ap);
	exit(eval);
}

#if 0
void
errx(int eval, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	_Vdomessage(0, fmt, ap);
	va_end(ap);
	exit(eval);
}
#endif

void
warn(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	_Vdomessage(1, fmt, ap);
	va_end(ap);
}

#if 0
void
warnx(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	_Vdomessage(0, fmt, ap);
	va_end(ap);
}
#endif
