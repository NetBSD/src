/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	@(#)cpu.h	7.7 (Berkeley) 6/27/91
 */

/*
 * Exported definitions unique to amiga/68k cpu support.
 */

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	COPY_SIGCODE		/* copy sigcode above user stack in exec */

/*
 * function vs. inline configuration;
 * these are defined to get generic functions
 * rather than inline or machine-dependent implementations
 */
#define	NEED_MINMAX		/* need {,i,l,ul}{min,max} functions */
#undef	NEED_FFS		/* don't need ffs function */
#undef	NEED_BCMP		/* don't need bcmp function */
#undef	NEED_STRLEN		/* don't need strlen function */

#define	cpu_exec(p)	/* nothing */
#define	cpu_wait(p)	/* nothing */

/*
 * Arguments to hardclock, softclock and gatherstats
 * encapsulate the previous machine state in an opaque
 * clockframe; for hp300, use just what the hardware
 * leaves on the stack.
 */
typedef struct intrframe {
	int	pc;
	int	ps;
} clockframe;

#define	CLKF_USERMODE(framep)	(((framep)->ps & PSL_S) == 0)
#define	CLKF_BASEPRI(framep)	(((framep)->ps & PSL_IPL7) == 0)
#define	CLKF_PC(framep)		((framep)->pc)


/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	need_resched()	{ want_resched++; aston(); }

/*
 * Give a profiling tick to the current process from the softclock
 * interrupt.  On hp300, request an ast to send us through trap(),
 * marking the proc as needing a profiling tick.
 */
#define	profile_tick(p, framep)	{ (p)->p_flag |= SOWEUPC; aston(); }

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)	aston()

#define aston() (astpending++)

int	astpending;		/* need to trap before returning to user mode */
int	want_resched;		/* resched() was called */


/*
 * simulated software interrupt register
 */
extern unsigned char ssir;

#define SIR_NET		0x1
#define SIR_CLOCK	0x2

#define siroff(x)	ssir &= ~(x)
#define setsoftnet()	ssir |= SIR_NET
#define setsoftclock()	ssir |= SIR_CLOCK


/*
 * The rest of this should probably be moved to ../amiga/amigacpu.h,
 * although some of it could probably be put into generic 68k headers.
 */

/* values for machineid (happen to be AFF_* settings of AttnFlags)
 * NOTE: '40 support does NOT YET exist! */
#define AMIGA_68020	(1L<<1)
#define AMIGA_68030	(1L<<2)
#define AMIGA_68040	(1L<<3)
#define AMIGA_68881	(1L<<4)
#define AMIGA_68882	(1L<<5)
#define	AMIGA_FPU40	(1L<<6)


/* values for mmutype (assigned for quick testing) */
#define	MMU_68030	-1	/* 68030 on-chip subset of 68851 */
#define	MMU_68851	1	/* Motorola 68851 */

/* values for cpuspeed (not really related to clock speed due to caches) */
#define	MHZ_8		1
#define	MHZ_16		2
#define	MHZ_25		3
#define	MHZ_33		4
#define	MHZ_50		6

#ifdef KERNEL
extern	int machineid, mmutype;
extern  char *chipmembase, *chipmemlimit;
extern  char *customchipbase, *customchiplimit;

/* what is this supposed to do? i.e. how is it different than startrtclock? 
   #define	enablertclock()
   
   Answer (MW): startrtclock is supposed to start the clock chip (to get an
   accurate uptime, enablertclock is called later (after *vital* stuff
   has been setup) to enable clock interrupts. Enabling clock interrupts
   at startrtclock-time can get you into big troubles...  */

#endif

/* physical memory sections */
#define CHIPMEMBASE	(0x00000000)
/* maximum for mapping, not the whole range is needed in physical equivalence */
#define CHIPMEMTOP	(0x00200000)
#define CHIPMEMSIZE	btoc(CHIPMEMTOP-CHIPMEMBASE)
/* CIA-A and CIA-B */
#define CIABASE		(0x00BFC000)
#define CIATOP		(0x00C00000)
#define CIASIZE		btoc(CIATOP-CIABASE)
#define CUSTOMBASE	(0x00DFE000)
#define CUSTOMTOP	(0x00E00000)
#define CUSTOMSIZE	btoc(CUSTOMTOP-CUSTOMBASE)
#ifdef A3000
#define SCSIBASE	(0x00DD0000)
#define SCSITOP		(0x00DD0000+AMIGA_PAGE_SIZE)
#define SCSISIZE	btoc(SCSITOP-SCSIBASE)
#endif

/* XXX only correct for A3000 memory map!
 * corresponds to address of last physical memory page, for A3000
 * this is always 0x08000000 - pagesize (== NBPS)
 */
#define MAXADDR		(0x08000000 - UPAGES)


/* Amiga specific mappings:
 *
 * phys-start	map-start	  phys-end	map-end		name
 *
 * 0x00000000	chipmembase	- 0x00200000	chipmemlimit	CHIP MEM
 * 0x00be0000	ciabase		- 0x00c00000	cialimit	CIA-B/CIA-A
 * 0x00d80000	customchipbase	- 0x00f00000	customchiplimit	CUSTOM/ZORRO2
 */
#define ISCHIPMEM(va) \
	((char *)(va) >= chipmembase && (char *)(va) < chipmemlimit)
#define	CHIPMEMV(pa)	((int)(pa)-CHIPMEMBASE+(int)chipmembase)
#define	CHIPMEMP(va)	((int)(va)-(int)chipmembase+CHIPMEMBASE)
#define	CHIPMEMPOFF(pa)	((int)(pa)-CHIPMEMBASE)
#define	CHIPMEMMAPSIZE	btoc(CHIPMEMTOP-CHIPMEMBASE)	/* 2mb */

#define ISCIA(va) \
	((char *)(va) >= ciabase && (char *)(va) < cialimit)
#define	CIAV(pa)	((int)(pa)-CIABASE+(int)ciabase)
#define	CIAP(va)	((int)(va)-(int)ciabase+CIABASE)
#define	CIAPOFF(pa)	((int)(pa)-CIABASE)
#define	CIAMAPSIZE	btoc(CIATOP-CIABASE)	/* 8k */

#define ISCUSTOMCHIP(va) \
	((char *)(va) >= customchipbase && (char *)(va) < customchiplimit)
#define	CUSTOMCHIPV(pa)	((int)(pa)-CUSTOMCHIPBASE+(int)customchipbase)
#define	CUSTOMCHIPP(va)	((int)(va)-(int)customchipbase+CUSTOMCHIPBASE)
#define	CUSTOMCHIPPOFF(pa)	((int)(pa)-CUSTOMCHIPBASE)
#define	CUSTOMCHIPMAPSIZE	btoc(CUSTOMCHIPTOP-CUSTOMCHIPBASE)	/* 1.5mb */


/*
 * 68851 and 68030 MMU
 */
#define	PMMU_LVLMASK	0x0007
#define	PMMU_INV	0x0400
#define	PMMU_WP		0x0800
#define	PMMU_ALV	0x1000
#define	PMMU_SO		0x2000
#define	PMMU_LV		0x4000
#define	PMMU_BE		0x8000
#define	PMMU_FAULT	(PMMU_WP|PMMU_INV)

/* 680X0 function codes */
#define	FC_USERD	1	/* user data space */
#define	FC_USERP	2	/* user program space */
#define	FC_SUPERD	5	/* supervisor data space */
#define	FC_SUPERP	6	/* supervisor program space */
#define	FC_CPU		7	/* CPU space */

/* fields in the 68020 cache control register */
#define	IC_ENABLE	0x0001	/* enable instruction cache */
#define	IC_FREEZE	0x0002	/* freeze instruction cache */
#define	IC_CE		0x0004	/* clear instruction cache entry */
#define	IC_CLR		0x0008	/* clear entire instruction cache */

/* additional fields in the 68030 cache control register */
#define	IC_BE		0x0010	/* instruction burst enable */
#define	DC_ENABLE	0x0100	/* data cache enable */
#define	DC_FREEZE	0x0200	/* data cache freeze */
#define	DC_CE		0x0400	/* clear data cache entry */
#define	DC_CLR		0x0800	/* clear entire data cache */
#define	DC_BE		0x1000	/* data burst enable */
#define	DC_WA		0x2000	/* write allocate */

#define	CACHE_ON	(DC_WA|DC_BE|DC_CLR|DC_ENABLE|IC_BE|IC_CLR|IC_ENABLE)
#define	CACHE_OFF	(DC_CLR|IC_CLR)
#define	CACHE_CLR	(CACHE_ON)
#define	IC_CLEAR	(DC_WA|DC_BE|DC_ENABLE|IC_BE|IC_CLR|IC_ENABLE)
#define	DC_CLEAR	(DC_WA|DC_BE|DC_CLR|DC_ENABLE|IC_BE|IC_ENABLE)
