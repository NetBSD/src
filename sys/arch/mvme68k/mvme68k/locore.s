/*	$NetBSD: locore.s,v 1.81 2001/06/10 17:03:01 scw Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * from: Utah $Hdr: locore.s 1.66 92/12/22$
 *
 *	@(#)locore.s	8.6 (Berkeley) 5/27/94
 */

#include "opt_compat_netbsd.h"
#include "opt_compat_svr4.h"
#include "opt_compat_sunos.h"
#include "opt_fpsp.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"

#include "assym.h"
#include <machine/asm.h>
#include <machine/trap.h>


/*
 * Temporary stack for a variety of purposes.
 * Try and make this the first thing is the data segment so it
 * is page aligned.  Note that if we overflow here, we run into
 * our text segment.
 */
	.data
	.space	NBPG
ASLOCAL(tmpstk)

ASLOCAL(bug_vbr)
	.long	0

#include <mvme68k/mvme68k/vectors.s>


/*
 * Macro to relocate a symbol, used before MMU is enabled.
 */
#define	_RELOC(var, ar)		\
	lea	var,ar

#define	RELOC(var, ar)		_RELOC(_C_LABEL(var), ar)
#define	ASRELOC(var, ar)	_RELOC(_ASM_LABEL(var), ar)

/*
 * Macro to call into the Bug ROM monitor
 */
#define	CALLBUG(func)	\
	trap #15; .short func

/*
 * Initialization
 *
 * The bootstrap loader loads us in starting at 0, and VBR is non-zero.
 * On entry, args on stack are boot device, boot filename, console unit,
 * boot flags (howto), boot device name, filesystem type name.
 */
BSS(lowram,4)
BSS(esym,4)

	.globl	_C_LABEL(edata)
	.globl	_C_LABEL(etext),_C_LABEL(end)


/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */
	.text
GLOBAL(kernel_text)

/*
 * start of kernel and .text!
 */
ASENTRY_NOPROFILE(start)
	movw	#PSL_HIGHIPL,%sr	| no interrupts
	movl	#0,%a5			| RAM starts at 0 (a5)
	movl	%sp@(4), %d7		| get boothowto
	movl	%sp@(8), %d6		| get bootaddr
	movl	%sp@(12),%d5		| get bootctrllun
	movl	%sp@(16),%d4		| get bootdevlun
	movl	%sp@(20),%d3		| get bootpart
	movl	%sp@(24),%d2		| get esyms

	RELOC(bootpart,%a0)
	movl	%d3, %a0@		| save bootpart
	RELOC(bootdevlun,%a0)
	movl	%d4, %a0@		| save bootdevlun
	RELOC(bootctrllun,%a0)
	movl	%d5, %a0@		| save booctrllun
	RELOC(bootaddr,%a0)
	movl	%d6, %a0@		| save bootaddr
	RELOC(boothowto,%a0)
	movl	%d7, %a0@		| save boothowto
	/* note: d3-d7 free, d2 still in use */

	ASRELOC(tmpstk, %a0)
	movl	%a0,%sp			| give ourselves a temporary stack

	RELOC(edata,%a0)		| clear out BSS
	movl	#_C_LABEL(end) - 4, %d0	| (must be <= 256 kB)
	subl	#_C_LABEL(edata), %d0
	lsrl	#2,%d0
1:	clrl	%a0@+
	dbra	%d0,1b

	RELOC(esym, %a0)
	movl	%d2,%a0@		| store end of symbol table
	/* d2 now free */
	RELOC(lowram, %a0)
	movl	%a5,%a0@		| store start of physical memory
	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| clear and disable on-chip cache(s)

	/* ask the Bug what we are... */
	clrl	%sp@-
	CALLBUG(MVMEPROM_GETBRDID)
	movl	%sp@+,%a1

	/* copy to a struct mvmeprom_brdid */
	movl	#MVMEPROM_BRDID_SIZE,%d0
	RELOC(boardid,%a0)
1:	movb	%a1@+,%a0@+
	subql	#1,%d0
	bne	1b

	/*
	 * Grab the model number from _boardid and use the value
	 * to setup machineid, cputype, and mmutype.
	 */
	clrl	%d0
	RELOC(boardid,%a1)
	movw	%a1@(MVMEPROM_BRDID_MODEL_OFFSET),%d0
	RELOC(machineid,%a0)
	movl	%d0,%a0@

	ASRELOC(Lbrdid2mach,%a0)
Lbrdmatch:
	cmpw	%a0@+,%d0
	beqs	Lgotmatch
	addw	#0x12,%a0		| Each entry is 20-2 bytes long
	tstw	%a0@
	bnes	Lbrdmatch

	/*
	 * If we fall to here, the board is not supported.
	 * Print a warning, then drop out to the Bug.
	 */
	movl	#Lenotconf,%sp@-
	movl	#Lnotconf,%sp@-
	CALLBUG(MVMEPROM_OUTSTRCRLF)
	addql	#8,%sp			| clean up stack after call

	CALLBUG(MVMEPROM_EXIT)
	/* NOTREACHED */

	.data
Lnotconf:
	.ascii	"Sorry, the kernel isn't configured for this model."
Lenotconf:
	.even

ASLOCAL(Lbrdid2mach)
#ifdef MVME147
	.word	MVME_147
	.word		CPU_68030
	.word		MMU_68030
	.word		FPU_68882
	.long		_C_LABEL(busaddrerr2030)
	.long		_C_LABEL(busaddrerr2030)
	.long		Linit147
#endif
#ifdef MVME162
	.word	MVME_162
	.word		CPU_68040
	.word		MMU_68040
	.word		FPU_68040
	.long		_C_LABEL(buserr40)
	.long		_C_LABEL(addrerr4060)
	.long		Linit1x2
#endif
#ifdef MVME167
	.word	MVME_167
	.word		CPU_68040
	.word		MMU_68040
	.word		FPU_68040
	.long		_C_LABEL(buserr40)
	.long		_C_LABEL(addrerr4060)
	.long		Linit1x7
#endif
#ifdef MVME172
	.word	MVME_172
	.word		CPU_68060
	.word		MMU_68040
	.word		FPU_68060
	.long		_C_LABEL(buserr60)
	.long		_C_LABEL(addrerr4060)
	.long		Linit1x2
#endif
#ifdef MVME177
	.word	MVME_177
	.word		CPU_68060
	.word		MMU_68040
	.word		FPU_68060
	.long		_C_LABEL(buserr60)
	.long		_C_LABEL(addrerr4060)
	.long		Linit1x7
#endif
	.word	0
	.text
	.even

/*
 * We have a match, so the kernel should support this board.
 * a0 points to the matching entry in Lbrdid2mach.
 */
Lgotmatch:
	movew	%a0@+,%d1		| Copy the CPU type
	extl	%d1
	RELOC(cputype,%a1)
	movel	%d1,%a1@
	movew	%a0@+,%d1		| Copy the MMU type
	extl	%d1
	RELOC(mmutype,%a1)
	movel	%d1,%a1@
	movew	%a0@+,%d1		| Copy the FPU type
	extl	%d1
	RELOC(fputype,%a1)
	movel	%d1,%a1@
	movel	%a0@+,%a2		| Fetch the bus error vector
	RELOC(vectab,%a1)
	movl	%a2,%a1@(8)
	movel	%a0@+,%a2		| Fetch the address error vector
	movl	%a2,%a1@(12)
	movel	%a0@,%a0		| Finally, the board-specific init code
	jmp	%a0@


#ifdef MVME147
Linit147:
	/* MVME-147 - 68030 CPU/MMU, 68882 FPU */
	/* XXXCDC SHUTUP 147 CALL */
	movb	#0, 0xfffe1026		| serial interrupt off
	movb	#0, 0xfffe1018		| timer 1 off
	movb	#0, 0xfffe1028		| ethernet off
	/* XXXCDC SHUTUP 147 CALL */

	/* Save our ethernet address */
	RELOC(mvme_ea, %a0)
	lea	0xfffe0778,%a1		| XXXCDC -- HARDWIRED HEX
	movb	#0x08,%a0@+
	clrb	%a0@+
	movb	#0x3e,%a0@+
	movql	#0x0f,%d0
	andb	%a1@+,%d0
	orb	#0x20,%d0
	movb	%d0,%a0@+
	movb	%a1@+,%a0@+
	movb	%a1@,%a0@

	/*
	 * Fix up the physical addresses of the MVME147's onboard
	 * I/O registers.
	 */
	RELOC(intiobase_phys, %a0);
	movl	#INTIOBASE147,%a0@
	RELOC(intiotop_phys, %a0);
	movl	#INTIOTOP147,%a0@

	/* initialise list of physical memory segments for pmap_bootstrap */
	RELOC(phys_seg_list, %a0)
	movl	%a5,%a0@		| phys_seg_list[0].ps_start
	movl	0xfffe0774,%d1		| End + 1 of onboard memory
	movl	%d1,%a0@(4)		| phys_seg_list[0].ps_end
	clrl	%a0@(8)			| phys_seg_list[0].ps_startpage

	/* offboard RAM */
	clrl	%a0@(0x0c)		| phys_seg_list[1].ps_start
	movl	#NBPG-1,%d0
	addl	0xfffe0764,%d0		| Start of offboard segment
	andl	#-NBPG,%d0		| Round up to page boundary
	beq	Lsavmaxmem		| Jump if none defined
	movl	#NBPG,%d1		| Note: implicit '+1'
	addl	0xfffe0768,%d1		| End of offboard segment
	andl	#-NBPG,%d1		| Round up to page boundary
	cmpl	%d1,%d0			| Quick and dirty validity check
	bcss	Loff_ok			| Yup, looks good.
	movel	%a0@(4),%d1		| Just use onboard RAM otherwise
	bras	Lsavmaxmem
Loff_ok:
	movl	%d0,%a0@(0x0c)		| phys_seg_list[1].ps_start
	movl	%d1,%a0@(0x10)		| phys_seg_list[1].ps_end
	clrl	%a0@(0x14)		| phys_seg_list[1].ps_startpage

	/*
	 * Offboard RAM needs to be cleared to zero to initialise parity
	 * on most VMEbus RAM cards. Without this, some cards will buserr
	 * when first read.
	 */
	movel	%d0,%a0			| offboard start address again.
Lclearoff:
	clrl	%a0@+			| zap a word
	cmpl	%a0,%d1			| reached end?
	bnes	Lclearoff

Lsavmaxmem:
	moveq	#PGSHIFT,%d2
	lsrl	%d2,%d1			| convert to page (click) number
	RELOC(maxmem, %a0)
	movl	%d1,%a0@		| save as maxmem
	jra	Lstart1
#endif

#if defined(MVME162) || defined(MVME172)
Linit1x2:
	/* MVME-162 - 68040 CPU/MMU/FPU */
	/* MVME-172 - 68060 CPU/MMU/FPU */

	/*
	 * Verify the user has removed the GPIO#0 jumper...
	 */
	btst	#0,0xfff4202d		| Clear == jumper installed
	jne	1f			| Ok.

	movl	#Le1x2jump,%sp@-
	movl	#L1x2jump,%sp@-
	CALLBUG(MVMEPROM_OUTSTRCRLF)
	addql	#8,%sp			| clean up stack after call

	CALLBUG(MVMEPROM_EXIT)
	/* NOTREACHED */

1:
	/*
	 * Determine if this board has a VMEchip2
	 */
	btst	#1,0xfff4202e		| VMEchip2 presence detect
	jne	2f			| Jump if it doesn't exist.

	/*
	 * Disable all interrupts from VMEchip2. This is especially
	 * useful when the kernel doesn't have the VMEchip2 driver
	 * configured. If we didn't do this, then we're at the mercy
	 * of whatever VMEchip2 interrupts the ROM set up. For example,
	 * hitting the ABORT switch could kill the system...
	 */
	movl	0xfff40088,%d0
	andl	#0xff7fffff,%d0		| Clear 'MIEN'
	movl	%d0,0xfff40088
2:
	/*
	 * Determine how much onboard memory is installed
	 */
	movql	#0x07,%d0
	andb	0xfff42024,%d0
	ASRELOC(Ldramsize1x2,%a0)
	movl	%a0@(%d0:w:4),%d1	| Lookup the size
	jeq	Lmemcquery		| Assume a MEMC chip if this is zero.
	jra	Lis1xx_common

	.data
	.even
	/*
	 * Table of DRAM register size values -> actual size in bytes
	 */
ASLOCAL(Ldramsize1x2)
	.long	0x00100000
	.long	0x00200000
	.long	0x00000000
	.long	0x00400000
	.long	0x00400000
	.long	0x00800000
	.long	0x00000000
	.long	0x01000000

L1x2jump:
	.ascii	"You must remove the jumper from pins 15-16 of J22 (mvme162)"
	.ascii	"or pins 1-2\015\012"
	.ascii	"J11 (mvme162-LX) first! See NetBSD/mvme68k FAQ for details."
Le1x2jump:
	.even

	.text
#endif

#if defined(MVME167) || defined(MVME177)
Linit1x7:
	/* MVME-167 - 68040 CPU/MMU/FPU */
	/* MVME-177 - 68060 CPU/MMU/FPU */

	/*
	 * Verify the user has removed the GPIO#0 jumper...
	 */
	movel	#0x01000000,%d0
	andl	0xfff40088,%d0		| Clear == jumper installed
	jne	1f			| Ok.

	movl	#Le1x7jump,%sp@-
	movl	#L1x7jump,%sp@-
	CALLBUG(MVMEPROM_OUTSTRCRLF)
	addql	#8,%sp			| clean up stack after call

	CALLBUG(MVMEPROM_EXIT)
	/* NOTREACHED */

1:
	/*
	 * Disable all interrupts from VMEchip2. This is especially
	 * useful when the kernel doesn't have the VMEchip2 driver
	 * configured. If we didn't do this, then we're at the mercy
	 * of whatever VMEchip2 interrupts the ROM set up. For example,
	 * hitting the ABORT switch could kill the system...
	 */
	movl	0xfff40088,%d0
	andl	#0xff7fffff,%d0		| Clear 'MIEN'
	movl	%d0,0xfff40088

	.data
	.even
L1x7jump:
	.ascii	"You must remove the jumper from pins 1-2 of J1!\015\012"
	.ascii	"See NetBSD/mvme68k FAQ for details."
Le1x7jump:
	.even

#endif

#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
Lmemcquery:
	/*
	 * Figure out the size of onboard DRAM by querying
	 * the memory controller ASIC(s)
	 */
	lea	0xfff43008,%a0		| MEMC040/MEMECC Controller #1
	bsr	memc040read
	movl	%d0,%d1

	lea	0xfff43108,%a0		| MEMC040/MEMECC Controller #2
	bsr	memc040read
	addl	%d0,%d1

Lis1xx_common:
	/* Save our ethernet address */
	RELOC(mvme_ea, %a0)
	lea	0xfffc1f2c,%a1
	movb	%a1@+,%a0@+
	movb	%a1@+,%a0@+
	movb	%a1@+,%a0@+
	movb	%a1@+,%a0@+
	movb	%a1@+,%a0@+
	movb	%a1@,%a0@

	/*
	 * Fix up the physical addresses of the onboard
	 * I/O registers.
	 */
	RELOC(intiobase_phys, %a0);
	movl	#INTIOBASE1xx,%a0@
	RELOC(intiotop_phys, %a0);
	movl	#INTIOTOP1xx,%a0@

	/*
	 * Initialise first physical memory segment with onboard RAM details
	 */
	RELOC(phys_seg_list, %a0)
	movl	%a5,%a0@		| phys_seg_list[0].ps_start
	movl	%d1,%a0@(4)		| phys_seg_list[0].ps_end
	clrl	%a0@(8)			| phys_seg_list[0].ps_startpage

	/* offboard RAM */
	clrl	%a0@(0x0c)		| phys_seg_list[1].ps_start
	movl	#NBPG-1,%d0
	addl	0xfffc0000,%d0		| Start of offboard segment
	andl	#-NBPG,%d0		| Round up to page boundary
	beq	Ldone1xx		| Jump if none defined
	movl	#NBPG,%d1		| Note: implicit '+1'
	addl	0xfffc0004,%d1		| End of offboard segment
	andl	#-NBPG,%d1		| Round up to page boundary
	cmpl	%d1,%d0			| Quick and dirty validity check
	bcss	Lramsave1xx		| Yup, looks good.
	movel	%a0@(4),%d1		| Just use onboard RAM otherwise
	bras	Ldone1xx

Lramsave1xx:
	movl	%d0,%a0@(0x0c)		| phys_seg_list[1].ps_start
	movl	%d1,%a0@(0x10)		| phys_seg_list[1].ps_end
	clrl	%a0@(0x14)		| phys_seg_list[1].ps_startpage

	/*
	 * Offboard RAM needs to be cleared to zero to initialise parity
	 * on most VMEbus RAM cards. Without this, some cards will buserr
	 * when first read.
	 */
	movel	%d0,%a0			| offboard start address again.
Lramclr1xx:
	clrl	%a0@+			| zap a word
	cmpl	%a0,%d1			| reached end?
	bnes	Lramclr1xx

Ldone1xx:
	moveq	#PGSHIFT,%d2
	lsrl	%d2,%d1			| convert to page (click) number
	RELOC(maxmem, %a0)
	movl	%d1,%a0@		| save as maxmem

	/* FALLTHROUGH to Lstart1 */
#endif


Lstart1:
/* initialize source/destination control registers for movs */
	moveq	#FC_USERD,%d0		| user space
	movc	%d0,%sfc		|   as source
	movc	%d0,%dfc		|   and destination of transfers
/*
 * configure kernel and proc0 VA space so we can get going
 */
#ifdef DDB
	RELOC(esym,%a0)			| end of static kernel text/data syms
	movl	%a0@,%d2
	jne	Lstart2
#endif
	movl	#_C_LABEL(end),%d2	| end of static kernel text/data
Lstart2:
	addl	#NBPG-1,%d2
	andl	#PG_FRAME,%d2		| round to a page
	movl	%d2,%a4
	addl	%a5,%a4			| convert to PA
	pea	%a5@			| firstpa
	pea	%a4@			| nextpa
	RELOC(pmap_bootstrap,%a0)
	jbsr	%a0@			| pmap_bootstrap(firstpa, nextpa)
	addql	#8,%sp

/*
 * Enable the MMU.
 * Since the kernel is mapped logical == physical, we just turn it on.
 */
	RELOC(Sysseg, %a0)		| system segment table addr
	movl	%a0@,%d1		| read value (a KVA)
	addl	%a5,%d1			| convert to PA
	RELOC(mmutype, %a0)
	cmpl	#MMU_68040,%a0@		| 68040?
	jne	Lmotommu1		| no, skip
	.long	0x4e7b1807		| movc d1,srp
	jra	Lstploaddone
Lmotommu1:
	RELOC(protorp, %a0)
	movl	#0x80000202,%a0@	| nolimit + share global + 4 byte PTEs
	movl	%d1,%a0@(4)		| + segtable address
	pmove	%a0@,%srp		| load the supervisor root pointer
	movl	#0x80000002,%a0@	| reinit upper half for CRP loads
Lstploaddone:
	RELOC(mmutype, %a0)
	cmpl	#MMU_68040,%a0@		| 68040?
	jne	Lmotommu2		| no, skip
	moveq	#0,%d0			| ensure TT regs are disabled
	.long	0x4e7b0004		| movc d0,itt0
	.long	0x4e7b0005		| movc d0,itt1
	.long	0x4e7b0006		| movc d0,dtt0
	.long	0x4e7b0007		| movc d0,dtt1
	.word	0xf4d8			| cinva bc
	.word	0xf518			| pflusha
	movl	#0x8000,%d0
	.long	0x4e7b0003		| movc d0,tc
#ifdef M68060
	RELOC(cputype, %a0)
	cmpl	#CPU_68060,%a0@		| 68060?
	jne	Lnot060cache
	movl	#1,%d0
	.long	0x4e7b0808		| movcl d0,pcr
	movl	#0xa0808000,%d0
	movc	%d0,%cacr		| enable store buffer, both caches
	jmp	Lenab1
Lnot060cache:
#endif
	movl	#0x80008000,%d0
	movc	%d0,%cacr		| turn on both caches
	jmp	Lenab1
Lmotommu2:
	movl	#0x82c0aa00,%sp@-	| value to load TC with
	pmove	%sp@,%tc		| load it

/*
 * Should be running mapped from this point on
 */
Lenab1:
/* Point the cpu VBR at our vector table */
	movc	%vbr,%d0		| Preserve Bug's VBR address
	movl	%d0,_ASM_LABEL(bug_vbr)
	movl	#_C_LABEL(vectab),%d0	| get our VBR address
	movc	%d0,%vbr
/* select the software page size now */
	lea	_ASM_LABEL(tmpstk),%sp	| temporary stack
	jbsr	_C_LABEL(uvm_setpagesize)  | select software page size
/* set kernel stack, user SP, and initial pcb */
	movl	_C_LABEL(proc0paddr),%a1 | get proc0 pcb addr
	lea	%a1@(USPACE-4),%sp	| set kernel stack to end of area
	lea	_C_LABEL(proc0),%a2	| initialize proc0.p_addr so that
	movl	%a1,%a2@(P_ADDR)	|   we don't deref NULL in trap()
	movl	#USRSTACK-4,%a2
	movl	%a2,%usp		| init user SP
	movl	%a1,_C_LABEL(curpcb)	| proc0 is running
	tstl	_C_LABEL(fputype)	| Have an FPU?
	jeq	Lenab2			| No, skip.
	clrl	%a1@(PCB_FPCTX)		| ensure null FP context
	movl	%a1,%sp@-
	jbsr	_C_LABEL(m68881_restore) | restore it (does not kill a1)
	addql	#4,%sp
Lenab2:
	cmpl	#MMU_68040,_C_LABEL(mmutype)	| 68040?
	jeq	Ltbia040		| yes, cache already on
	pflusha
	movl	#CACHE_ON,%d0
	movc	%d0,%cacr		| clear cache(s)
	jra	Lenab3
Ltbia040:
	.word	0xf518
Lenab3:
/*
 * final setup for C code:
 * Create a fake exception frame so that cpu_fork() can copy it.
 * main() nevers returns; we exit to user mode from a forked process
 * later on.
 */
	jbsr	_C_LABEL(mvme68k_init)	| additional pre-main initialization
	movw	#PSL_LOWIPL,%sr		| lower SPL
	clrw	%sp@-			| vector offset/frame type
	clrl	%sp@-			| PC - filled in by "execve"
	movw	#PSL_USER,%sp@-		| in user mode
	clrl	%sp@-			| stack adjust count and padding
	lea	%sp@(-64),%sp		| construct space for D0-D7/A0-A7
	lea	_C_LABEL(proc0),%a0	| save pointer to frame
	movl	%sp,%a0@(P_MD_REGS)	|   in proc0.p_md.md_regs

	jra	_C_LABEL(main)		| main()

#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
/*
 * Probe for a memory controller ASIC (MEMC040 or MEMECC) at the
 * address in a0. If found, return the size in bytes of any RAM
 * controlled by the ASIC in d0. Otherwise return zero.
 */
ASLOCAL(memc040read)
	moveml	%d1-%d2/%a1-%a2,%sp@-	| save scratch regs
	movc	%vbr,%d2		| Save vbr
	RELOC(vectab,%a2)		| Install our own vectab, temporarily
	movc	%a2,%vbr
	ASRELOC(Lmemc040berr,%a1)	| get address of bus error handler
	movl	%a2@(8),%sp@-		| Save current bus error handler addr
	movl	%a1,%a2@(8)		| Install our own handler
	movl	%sp,%d0			| Save current stack pointer value
	movql	#0x07,%d1
	andb	%a0@,%d1		| Access MEMC040/MEMECC
	movl	#0x400000,%d0
	lsll	%d1,%d0			| Convert to memory size, in bytes
Lmemc040ret:
	movc	%d2,%vbr		| Restore original vbr
	movl	%sp@+,%a2@(8)		| Restore original bus error handler
	moveml  %sp@+,%d1-%d2/%a1-%a2
	rts
/*
 * If the memory controller doesn't exist, we get a bus error trying
 * to access a0@ above. Control passes here, where we flag 'no bytes',
 * ditch the exception frame and return as normal.
 */
Lmemc040berr:
	movl	%d0,%sp			| Get rid of the exception frame
	movql	#0,%d0			| No ASIC at this location, then!
	bra	Lmemc040ret		| Done
#endif

/*
 * proc_trampoline: call function in register a2 with a3 as an arg
 * and then rei.
 */
GLOBAL(proc_trampoline)
	movl    %a3,%sp@-		| push function arg
	jbsr    %a2@			| call function
	addql   #4,%sp			| pop arg
	movl    %sp@(FR_SP),%a0		| grab and load
	movl    %a0,%usp		|   user SP
	moveml  %sp@+,#0x7FFF		| restore most user regs
	addql   #8,%sp			| toss SP and stack adjust
	jra     _ASM_LABEL(rei)         | and return

/*
 * Trap/interrupt vector routines
 */ 
#include <m68k/m68k/trap_subr.s>

#if defined(M68040) || defined(M68060)
ENTRY_NOPROFILE(addrerr4060)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%sp@(FR_HW+8),%sp@-
	clrl	%sp@-			| dummy code
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
#endif

#if defined(M68060)
ENTRY_NOPROFILE(buserr60)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movel	%sp@(FR_HW+12),%d0	| FSLW
	btst	#2,%d0			| branch prediction error?
	jeq	Lnobpe			
	movc	%cacr,%d2
	orl	#IC60_CABC,%d2		| clear all branch cache entries
	movc	%d2,%cacr
	movl	%d0,%d1
	andl	#0x7ffd,%d1
	jeq	_ASM_LABEL(faultstkadjnotrap2)
Lnobpe:
| we need to adjust for misaligned addresses
	movl	%sp@(FR_HW+8),%d1	| grab VA
	btst	#27,%d0			| check for mis-aligned access
	jeq	Lberr3			| no, skip
	addl	#28,%d1			| yes, get into next page
					| operand case: 3,
					| instruction case: 4+12+12
	andl	#PG_FRAME,%d1		| and truncate
Lberr3:
	movl	%d1,%sp@-
	movl	%d0,%sp@-		| code is FSLW now.
	andw	#0x1f80,%d0 
	jeq	Lberr60			| it is a bus error
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lberr60:
	tstl	_C_LABEL(nofault)	| catch bus error?
	jeq	Lisberr			| no, handle as usual
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
#endif
#if defined(M68040)
ENTRY_NOPROFILE(buserr40)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%sp@(FR_HW+20),%d1	| get fault address
	moveq	#0,%d0
	movw	%sp@(FR_HW+12),%d0	| get SSW
	btst	#11,%d0			| check for mis-aligned
	jeq	Lbe1stpg		| no skip
	addl	#3,%d1			| get into next page
	andl	#PG_FRAME,%d1		| and truncate
Lbe1stpg:
	movl	%d1,%sp@-		| pass fault address.
	movl	%d0,%sp@-		| pass SSW as code
	btst	#10,%d0			| test ATC
	jeq	Lberr40			| it is a bus error
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lberr40:
	tstl	_C_LABEL(nofault)	| catch bus error?
	jeq	Lisberr			| no, handle as usual
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
#endif

#if defined(M68020) || defined(M68030)
ENTRY_NOPROFILE(busaddrerr2030)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	moveq	#0,%d0
	movw	%sp@(FR_HW+10),%d0	| grab SSW for fault processing
	btst	#12,%d0			| RB set?
	jeq	LbeX0			| no, test RC
	bset	#14,%d0			| yes, must set FB
	movw	%d0,%sp@(FR_HW+10)	| for hardware too
LbeX0:
	btst	#13,%d0			| RC set?
	jeq	LbeX1			| no, skip
	bset	#15,%d0			| yes, must set FC
	movw	%d0,%sp@(FR_HW+10)	| for hardware too
LbeX1:
	btst	#8,%d0			| data fault?
	jeq	Lbe0			| no, check for hard cases
	movl	%sp@(FR_HW+16),%d1	| fault address is as given in frame
	jra	Lbe10			| thats it
Lbe0:
	btst	#4,%sp@(FR_HW+6)	| long (type B) stack frame?
	jne	Lbe4			| yes, go handle
	movl	%sp@(FR_HW+2),%d1	| no, can use save PC
	btst	#14,%d0			| FB set?
	jeq	Lbe3			| no, try FC
	addql	#4,%d1			| yes, adjust address
	jra	Lbe10			| done
Lbe3:
	btst	#15,%d0			| FC set?
	jeq	Lbe10			| no, done
	addql	#2,%d1			| yes, adjust address
	jra	Lbe10			| done
Lbe4:
	movl	%sp@(FR_HW+36),%d1	| long format, use stage B address
	btst	#15,%d0			| FC set?
	jeq	Lbe10			| no, all done
	subql	#2,%d1			| yes, adjust address
Lbe10:
	movl	%d1,%sp@-		| push fault VA
	movl	%d0,%sp@-		| and padded SSW
	movw	%sp@(FR_HW+8+6),%d0	| get frame format/vector offset
	andw	#0x0FFF,%d0		| clear out frame format
	cmpw	#12,%d0			| address error vector?
	jeq	Lisaerr			| yes, go to it
	movl	%d1,%a0			| fault address
	movl	%sp@,%d0		| function code from ssw
	btst	#8,%d0			| data fault?
	jne	Lbe10a
	movql	#1,%d0			| user program access FC
					| (we dont seperate data/program)
	btst	#5,%sp@(FR_HW+8)	| supervisor mode?
	jeq	Lbe10a			| if no, done
	movql	#5,%d0			| else supervisor program access
Lbe10a:
	ptestr	%d0,%a0@,#7		| do a table search
	pmove	%psr,%sp@		| save result
	movb	%sp@,%d1
	btst	#2,%d1			| invalid (incl. limit viol. and berr)?
	jeq	Lmightnotbemerr		| no -> wp check
	btst	#7,%d1			| is it MMU table berr?
	jne	Lisberr1		| yes, needs not be fast.
Lismerr:
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lmightnotbemerr:
	btst	#3,%d1			| write protect bit set?
	jeq	Lisberr1		| no: must be bus error
	movl	%sp@,%d0		| ssw into low word of d0
	andw	#0xc0,%d0		| Write protect is set on page:
	cmpw	#0x40,%d0		| was it read cycle?
	jne	Lismerr			| no, was not WPE, must be MMU fault
	jra	Lisberr1		| real bus err needs not be fast.
Lisaerr:
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lisberr1:
	clrw	%sp@			| re-clear pad word
	tstl	_C_LABEL(nofault)	| catch bus error?
	jeq	Lisberr			| no, handle as usual
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
#endif /* M68020 || M68030 */

Lisberr:				| also used by M68040/60
	movl	#T_BUSERR,%sp@-		| mark bus error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it

/*
 * FP exceptions.
 */
ENTRY_NOPROFILE(fpfline)
#if defined(M68040)
	cmpl	#FPU_68040,_C_LABEL(fputype) | 68040 FPU?
	jne	Lfp_unimp		| no, skip FPSP
	cmpw	#0x202c,%sp@(6)		| format type 2?
	jne	_C_LABEL(illinst)	| no, not an FP emulation
#ifdef FPSP
	jmp	_ASM_LABEL(fpsp_unimp)	| yes, go handle it
#else
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save registers
	moveq	#T_FPEMULD,%d0		| denote as FP emulation trap
	jra	_ASM_LABEL(fault)	| do it
#endif
Lfp_unimp:
#endif /* M68040 */
	jra	_C_LABEL(illinst)

ENTRY_NOPROFILE(fpunsupp)
#if defined(M68040)
	cmpl	#FPU_68040,_C_LABEL(fputype) | 68040 FPU?
	jne	Lfp_unsupp		| No, skip FPSP
#ifdef FPSP
	jmp	_ASM_LABEL(fpsp_unsupp)	| yes, go handle it
#else
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save registers
	moveq	#T_FPEMULD,%d0		| denote as FP emulation trap
	jra	_ASM_LABEL(fault)	| do it
#endif
Lfp_unsupp:
#endif /* M68040 */
	jra	_C_LABEL(illinst)

/*
 * Handles all other FP coprocessor exceptions.
 * Note that since some FP exceptions generate mid-instruction frames
 * and may cause signal delivery, we need to test for stack adjustment
 * after the trap call.
 */
ENTRY_NOPROFILE(fpfault)
	clrl	%sp@-		| stack adjust count
	moveml	#0xFFFF,%sp@-	| save user registers
	movl	%usp,%a0	| and save
	movl	%a0,%sp@(FR_SP)	|   the user stack pointer
	clrl	%sp@-		| no VA arg
	movl	_C_LABEL(curpcb),%a0 | current pcb
	lea	%a0@(PCB_FPCTX),%a0 | address of FP savearea
	fsave	%a0@		| save state
#if defined(M68040) || defined(M68060)
	/* always null state frame on 68040, 68060 */
	cmpl	#FPU_68040,_C_LABEL(fputype)
	jle	Lfptnull
#endif
	tstb	%a0@		| null state frame?
	jeq	Lfptnull	| yes, safe
	clrw	%d0		| no, need to tweak BIU
	movb	%a0@(1),%d0	| get frame size
	bset	#3,%a0@(0,%d0:w) | set exc_pend bit of BIU
Lfptnull:
	fmovem	%fpsr,%sp@-	| push fpsr as code argument
	frestore %a0@		| restore state
	movl	#T_FPERR,%sp@-	| push type arg
	jra	_ASM_LABEL(faultstkadj) | call trap and deal with stack cleanup


/*
 * Other exceptions only cause four and six word stack frame and require
 * no post-trap stack adjustment.
 */

ENTRY_NOPROFILE(badtrap)
	moveml	#0xC0C0,%sp@-		| save scratch regs
	movw	%sp@(22),%sp@-		| push exception vector info
	clrw	%sp@-
	movl	%sp@(22),%sp@-		| and PC
	jbsr	_C_LABEL(straytrap)	| report
	addql	#8,%sp			| pop args
	moveml	%sp@+,#0x0303		| restore regs
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(trap0)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%d0,%sp@-		| push syscall number
	jbsr	_C_LABEL(syscall)	| handle it
	addql	#4,%sp			| pop syscall arg
	tstl	_C_LABEL(astpending)
	jne	Lrei2
	tstb	_C_LABEL(ssir)
	jne	Ltrap1
	movw	#SPL1,%sr
	tstb	_C_LABEL(ssir)
	jeq	Lsir1
Ltrap1:
	movl	%sp@(FR_SP),%a0		| grab and restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| restore most registers
	addql	#8,%sp			| pop SP and stack adjust
	rte

/*
 * Trap 12 is the entry point for the cachectl "syscall" (both HPUX & BSD)
 *	cachectl(command, addr, length)
 * command in d0, addr in a1, length in d1
 */
ENTRY_NOPROFILE(trap12)
	movl	_C_LABEL(curproc),%sp@-	| push curproc pointer
	movl	%d1,%sp@-		| push length
	movl	%a1,%sp@-		| push addr
	movl	%d0,%sp@-		| push command
	jbsr	_C_LABEL(cachectl1)	| do it
	lea	%sp@(16),%sp		| pop args
	jra	_ASM_LABEL(rei)		| all done

/*
 * Trace (single-step) trap.  Kernel-mode is special.
 * User mode traps are simply passed on to trap().
 */
ENTRY_NOPROFILE(trace)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-
	moveq	#T_TRACE,%d0

	| Check PSW and see what happen.
	|   T=0 S=0	(should not happen)
	|   T=1 S=0	trace trap from user mode
	|   T=0 S=1	trace trap on a trap instruction
	|   T=1 S=1	trace trap from system mode (kernel breakpoint)

	movw	%sp@(FR_HW),%d1		| get PSW
	notw	%d1			| XXX no support for T0 on 680[234]0
	andw	#PSL_TS,%d1		| from system mode (T=1, S=1)?
	jeq	Lkbrkpt			| yes, kernel breakpoint
	jra	_ASM_LABEL(fault)	| no, user-mode fault

/*
 * Trap 15 is used for:
 *	- GDB breakpoints (in user programs)
 *	- KGDB breakpoints (in the kernel)
 *	- trace traps for SUN binaries (not fully supported yet)
 * User mode traps are simply passed to trap().
 */
ENTRY_NOPROFILE(trap15)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-
	moveq	#T_TRAP15,%d0
	movw	%sp@(FR_HW),%d1		| get PSW
	andw	#PSL_S,%d1		| from system mode?
	jne	Lkbrkpt			| yes, kernel breakpoint
	jra	_ASM_LABEL(fault)	| no, user-mode fault

Lkbrkpt: | Kernel-mode breakpoint or trace trap. (d0=trap_type)
	| Save the system sp rather than the user sp.
	movw	#PSL_HIGHIPL,%sr	| lock out interrupts
	lea	%sp@(FR_SIZE),%a6	| Save stack pointer
	movl	%a6,%sp@(FR_SP)		|  from before trap

	| If were are not on tmpstk switch to it.
	| (so debugger can change the stack pointer)
	movl	%a6,%d1
	cmpl	#_ASM_LABEL(tmpstk),%d1
	jls	Lbrkpt2			| already on tmpstk
	| Copy frame to the temporary stack
	movl	%sp,%a0			| a0=src
	lea	_ASM_LABEL(tmpstk)-96,%a1 | a1=dst
	movl	%a1,%sp			| sp=new frame
	movql	#FR_SIZE,%d1
Lbrkpt1:
	movl	%a0@+,%a1@+
	subql	#4,%d1
	bgt	Lbrkpt1

Lbrkpt2:
	| Call the trap handler for the kernel debugger.
	| Do not call trap() to do it, so that we can
	| set breakpoints in trap() if we want.  We know
	| the trap type is either T_TRACE or T_BREAKPOINT.
	| If we have both DDB and KGDB, let KGDB see it first,
	| because KGDB will just return 0 if not connected.
	| Save args in d2, a2
	movl	%d0,%d2			| trap type
	movl	%sp,%a2			| frame ptr
#ifdef KGDB
	| Let KGDB handle it (if connected)
	movl	%a2,%sp@-		| push frame ptr
	movl	%d2,%sp@-		| push trap type
	jbsr	_C_LABEL(kgdb_trap)	| handle the trap
	addql	#8,%sp			| pop args
	cmpl	#0,%d0			| did kgdb handle it?
	jne	Lbrkpt3			| yes, done
#endif
#ifdef DDB
	| Let DDB handle it
	movl	%a2,%sp@-		| push frame ptr
	movl	%d2,%sp@-		| push trap type
	jbsr	_C_LABEL(kdb_trap)	| handle the trap
	addql	#8,%sp			| pop args
#endif
	/* Sun 3 drops into PROM here. */
Lbrkpt3:
	| The stack pointer may have been modified, or
	| data below it modified (by kgdb push call),
	| so push the hardware frame at the current sp
	| before restoring registers and returning.

	movl	%sp@(FR_SP),%a0		| modified sp
	lea	%sp@(FR_SIZE),%a1	| end of our frame
	movl	%a1@-,%a0@-		| copy 2 longs with
	movl	%a1@-,%a0@-		| ... predecrement
	movl	%a0,%sp@(FR_SP)		| sp = h/w frame
	moveml	%sp@+,#0x7FFF		| restore all but sp
	movl	%sp@,%sp		| ... and sp
	rte				| all done

/*
 * Use common m68k sigreturn routine.
 */
#include <m68k/m68k/sigreturn.s>

/*
 * Interrupt handlers.
 *
 * For auto-vectored interrupts, the CPU provides the
 * vector 0x18+level.
 *
 * intrhand_autovec is the entry point for auto-vectored
 * interrupts.
 *
 * For vectored interrupts, we pull the pc, evec, and exception frame
 * and pass them to the vectored interrupt dispatcher.  The vectored
 * interrupt dispatcher will deal with strays.
 *
 * intrhand_vectored is the entry point for vectored interrupts.
 */

#define INTERRUPT_SAVEREG	moveml  #0xC0C0,%sp@-
#define INTERRUPT_RESTOREREG	moveml  %sp@+,#0x0303

ENTRY_NOPROFILE(intrhand_autovec)	/* Levels 0 through 7 */
	INTERRUPT_SAVEREG
	lea	%sp@(16),%a1		| get pointer to frame
	movl	%a1,%sp@-
	movl	%sp@(26),%d0
	movl	%d0,%sp@-		| push exception vector
	jbsr	_C_LABEL(isrdispatch_autovec) | call dispatcher
	addql	#4,%sp
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(intrhand_vectored)
	INTERRUPT_SAVEREG
	lea	%sp@(16),%a1		| get pointer to frame
	movl	%a1,%sp@-
	movw	%sp@(26),%d0
	movl	%d0,%sp@-		| push exception vector info
	movl	%sp@(26),%sp@-		| and PC
	jbsr	_C_LABEL(isrdispatch_vectored) | call dispatcher
	lea	%sp@(12),%sp		| pop value args
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)		| all done

#undef INTERRUPT_SAVEREG
#undef INTERRUPT_RESTOREREG

/*
 * Emulation of VAX REI instruction.
 *
 * This code deals with checking for and servicing ASTs
 * (profiling, scheduling) and software interrupts (network, softclock).
 * We check for ASTs first, just like the VAX.  To avoid excess overhead
 * the T_ASTFLT handling code will also check for software interrupts so we
 * do not have to do it here.  After identifing that we need an AST we
 * drop the IPL to allow device interrupts.
 *
 * This code is complicated by the fact that sendsig may have been called
 * necessitating a stack cleanup.
 *
 * Note that 'ssir' is zero when a soft interrupt is pending, otherwise it
 * is non-zero. This is because it is tested elsewhere using the m68k `tas'
 * instruction.
 */

BSS(ssir,1)

ASENTRY_NOPROFILE(rei)
	tstl	_C_LABEL(astpending)	| AST pending?
	jeq	Lchksir			| no, go check for SIR
Lrei1:
	btst	#5,%sp@			| yes, are we returning to user mode?
	jne	Lchksir			| no, go check for SIR
	movw	#PSL_LOWIPL,%sr		| lower SPL
	clrl	%sp@-			| stack adjust
	moveml	#0xFFFF,%sp@-		| save all registers
	movl	%usp,%a1		| including
	movl	%a1,%sp@(FR_SP)		|    the users SP
Lrei2:
	clrl	%sp@-			| VA == none
	clrl	%sp@-			| code == none
	movl	#T_ASTFLT,%sp@-		| type == async system trap
	jbsr	_C_LABEL(trap)		| go handle it
	lea	%sp@(12),%sp		| pop value args
	movl	%sp@(FR_SP),%a0		| restore user SP
	movl	%a0,%usp		|   from save area
	movw	%sp@(FR_ADJ),%d0	| need to adjust stack?
	jne	Laststkadj		| yes, go to it
	moveml	%sp@+,#0x7FFF		| no, restore most user regs
	addql	#8,%sp			| toss SP and stack adjust
	rte				| and do real RTE
Laststkadj:
	lea	%sp@(FR_HW),%a1		| pointer to HW frame
	addql	#8,%a1			| source pointer
	movl	%a1,%a0			| source
	addw	%d0,%a0			|  + hole size = dest pointer
	movl	%a1@-,%a0@-		| copy
	movl	%a1@-,%a0@-		|  8 bytes
	movl	%a0,%sp@(FR_SP)		| new SSP
	moveml	%sp@+,#0x7FFF		| restore user registers
	movl	%sp@,%sp		| and our SP
	rte				| and do real RTE
Lchksir:
	tstb	_C_LABEL(ssir)		| SIR pending?
	jne	Ldorte			| no, all done
	movl	%d0,%sp@-		| need a scratch register
	movw	%sp@(4),%d0		| get SR
	andw	#PSL_IPL7,%d0		| mask all but IPL
	jne	Lnosir			| came from interrupt, no can do
	movl	%sp@+,%d0		| restore scratch register
Lgotsir:
	movw	#SPL1,%sr		| prevent others from servicing int
	tstb	_C_LABEL(ssir)		| too late?
	jne	Ldorte			| yes, oh well...
	clrl	%sp@-			| stack adjust
	moveml	#0xFFFF,%sp@-		| save all registers
	movl	%usp,%a1		| including
	movl	%a1,%sp@(FR_SP)		|    the users SP
Lsir1:
	clrl	%sp@-			| VA == none
	clrl	%sp@-			| code == none
	movl	#T_SSIR,%sp@-		| type == software interrupt
	jbsr	_C_LABEL(trap)		| go handle it
	lea	%sp@(12),%sp		| pop value args
	movl	%sp@(FR_SP),%a0		| restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| and all remaining registers
	addql	#8,%sp			| pop SP and stack adjust
	rte
Lnosir:
	movl	%sp@+,%d0		| restore scratch register
Ldorte:
	rte				| real return

/*
 * Set processor priority level calls.  Most are implemented with
 * inline asm expansions.  However, spl0 requires special handling
 * as we need to check for our emulated software interrupts.
 */

ENTRY(mvme68k_dossir)
	subql	#4,%sp			| make room for RTE frame
	movl	%sp@(4),%sp@(2)		| position return address
	clrw	%sp@(6)			| set frame type 0
	movw	#PSL_LOWIPL,%sp@	| and new SR
	jra	Lgotsir			| go handle it

/*
 * Use common m68k sigcode.
 */
#include <m68k/m68k/sigcode.s>
#ifdef COMPAT_SUNOS
#include <m68k/m68k/sunos_sigcode.s>
#endif
#ifdef COMPAT_SVR4
#include <m68k/m68k/svr4_sigcode.s>
#endif

/*
 * Primitives
 */ 

/*
 * Use common m68k support routines.
 */
#include <m68k/m68k/support.s>

/*
 * Use common m68k process manipulation routines.
 */
#include <m68k/m68k/proc_subr.s>

	.data
GLOBAL(curpcb)
GLOBAL(masterpaddr)		| XXXcompatibility (debuggers)
	.long	0

ASLOCAL(mdpflag)
	.byte	0		| copy of proc md_flags low byte
#ifdef __ELF__
	.align	4
#else
	.align	2
#endif

ASBSS(nullpcb,SIZEOF_PCB)

/*
 * At exit of a process, do a switch for the last time.
 * Switch to a safe stack and PCB, and select a new process to run.  The
 * old stack and u-area will be freed by the reaper.
 *
 * MUST BE CALLED AT SPLHIGH!
 */
ENTRY(switch_exit)
	movl    %sp@(4),%a0
	/* save state into garbage pcb */
	movl    #_ASM_LABEL(nullpcb),_C_LABEL(curpcb)
	lea     _ASM_LABEL(tmpstk),%sp	| goto a tmp stack

	/* Schedule the vmspace and stack to be freed. */
	movl	%a0,%sp@-		| exit2(p)
	jbsr	_C_LABEL(exit2)
	lea	%sp@(4),%sp		| pop args

#if defined(LOCKDEBUG)
	/* Acquire sched_lock */ 
	jbsr	_C_LABEL(sched_lock_idle)
#endif

	jra	_C_LABEL(cpu_switch)

/*
 * When no processes are on the runq, Swtch branches to Idle
 * to wait for something to come ready.
 */
ASENTRY_NOPROFILE(Idle)
#if defined(LOCKDEBUG)
	/* Release sched_lock */
	jbsr	_C_LABEL(sched_unlock_idle)
#endif
	stop	#PSL_LOWIPL
	movw	#PSL_HIGHIPL,%sr
#if defined(LOCKDEBUG)
	/* Acquire sched_lock */
	jbsr	_C_LABEL(sched_lock_idle)
#endif
	movl    _C_LABEL(sched_whichqs),%d0
	jeq     _ASM_LABEL(Idle)
	jra	Lsw1

Lbadsw:
	PANIC("switch")
	/*NOTREACHED*/

/*
 * cpu_switch()
 *
 * NOTE: With the new VM layout we now no longer know if an inactive
 * user's PTEs have been changed (formerly denoted by the SPTECHG p_flag
 * bit).  For now, we just always flush the full ATC.
 */
ENTRY(cpu_switch)
	movl	_C_LABEL(curpcb),%a0	| current pcb
	movw	%sr,%a0@(PCB_PS)	| save sr before changing ipl
#ifdef notyet
	movl	_C_LABEL(curproc),%sp@-	| remember last proc running
#endif
	clrl	_C_LABEL(curproc)

	/*
	 * Find the highest-priority queue that isn't empty,
	 * then take the first proc from that queue.
	 */
	movl    _C_LABEL(sched_whichqs),%d0
	jeq     _ASM_LABEL(Idle)
Lsw1:
	/*
	 * Interrupts are blocked, sched_lock is held.  If
	 * we come here via Idle, %d0 contains the contents
	 * of a non-zero sched_whichqs.
	 */
	movl    %d0,%d1
	negl    %d0
	andl    %d1,%d0
	bfffo   %d0{#0:#32},%d1
	eorib   #31,%d1

	movl    %d1,%d0
	lslb    #3,%d1			| convert queue number to index
	addl    #_C_LABEL(sched_qs),%d1	| locate queue (q)
	movl    %d1,%a1
	movl    %a1@(P_FORW),%a0	| p = q->p_forw
	cmpal   %d1,%a0			| anyone on queue?
	jeq     Lbadsw                  | no, panic
#ifdef DIAGNOSTIC
	tstl	%a0@(P_WCHAN)
	jne	Lbadsw
	cmpb	#SRUN,%a0@(P_STAT)
	jne	Lbadsw
#endif
	movl    %a0@(P_FORW),%a1@(P_FORW) | q->p_forw = p->p_forw
	movl    %a0@(P_FORW),%a1	| n = p->p_forw
	movl    %d1,%a1@(P_BACK)	| n->p_back = q
	cmpal   %d1,%a1			| anyone left on queue?
	jne     Lsw2			| yes, skip
	movl    _C_LABEL(sched_whichqs),%d1
	bclr    %d0,%d1			| no, clear bit
	movl    %d1,_C_LABEL(sched_whichqs)
Lsw2:
	/* p->p_cpu initialized in fork1() for single-processor */
	movb	#SONPROC,%a0@(P_STAT)	| p->p_stat = SONPROC
	movl	%a0,_C_LABEL(curproc)
	clrl	_C_LABEL(want_resched)
#ifdef notyet
	movl	%sp@+,%a1
	cmpl	%a0,%a1			| switching to same proc?
	jeq	Lswdone			| yes, skip save and restore
#endif
	/*
	 * Save state of previous process in its pcb.
	 */
	movl	_C_LABEL(curpcb),%a1
	moveml	#0xFCFC,%a1@(PCB_REGS)	| save non-scratch registers
	movl	%usp,%a2		| grab USP (a2 has been saved)
	movl	%a2,%a1@(PCB_USP)	| and save it

	tstl	_C_LABEL(fputype)	| Do we have an FPU?
	jeq	Lswnofpsave		| No  Then don't attempt save.
	lea	%a1@(PCB_FPCTX),%a2	| pointer to FP save area
	fsave	%a2@			| save FP state
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	Lsavfp60                
#endif  
	tstb	%a2@			| null state frame?
	jeq	Lswnofpsave		| yes, all done
	fmovem	%fp0-%fp7,%a2@(FPF_REGS) | save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,%a2@(FPF_FPCR) | save FP control registers
#if defined(M68060)
	jra	Lswnofpsave 
Lsavfp60:
#endif  
#endif  
#if defined(M68060)
	tstb	%a2@(2)			| null state frame?
	jeq	Lswnofpsave		| yes, all done 
	fmovem	%fp0-%fp7,%a2@(FPF_REGS) | save FP general registers 
	fmovem	%fpcr,%a2@(FPF_FPCR)	| save FP control registers
	fmovem	%fpsr,%a2@(FPF_FPSR)
	fmovem	%fpi,%a2@(FPF_FPI)
#endif
Lswnofpsave:

	clrl	%a0@(P_BACK)		| clear back link
	/* low byte of p_md.md_flags */
	movb	%a0@(P_MD_FLAGS+3),_ASM_LABEL(mdpflag)
	movl	%a0@(P_ADDR),%a1	| get p_addr
	movl	%a1,_C_LABEL(curpcb)

#if defined(LOCKDEBUG)
	/*
	 * Done mucking with the run queues, release the
	 * scheduler lock, but keep interrupts out.
	 */
	movl	%a0,%sp@-		| not args...
	movl	%a1,%sp@-		| ...just saving
	jbsr	_C_LABEL(sched_unlock_idle)
	movl	%sp@+,%a1
	movl	%sp@+,%a0
#endif

	/*
	 * Activate process's address space.
	 * XXX Should remember the last USTP value loaded, and call this
	 * XXX only of it has changed.
	 */
	pea	%a0@			| push proc
	jbsr	_C_LABEL(pmap_activate)	| pmap_activate(p)
	addql	#4,%sp
	movl	_C_LABEL(curpcb),%a1	| restore p_addr

	lea     _ASM_LABEL(tmpstk),%sp	| now goto a tmp stack for NMI

	moveml	%a1@(PCB_REGS),#0xFCFC	| and registers
	movl	%a1@(PCB_USP),%a0
	movl	%a0,%usp		| and USP

	tstl	_C_LABEL(fputype)	| If we don't have an FPU,
	jeq	Lnofprest		|  don't try to restore it.
	lea	%a1@(PCB_FPCTX),%a0	| pointer to FP save area
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	Lresfp60rest1
#endif  
	tstb	%a0@			| null state frame?
	jeq	Lresfprest		| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr/%fpsr/%fpi | restore FP control registers
	fmovem	%a0@(FPF_REGS),%fp0-%fp7	| restore FP general registers
Lresfprest:
#endif
	frestore %a0@			| restore state
Lnofprest:
	movw	%a1@(PCB_PS),%sr	| restore PS
	moveq	#1,%d0			| return 1 (for alternate returns)
	rts

#if defined(M68060)
Lresfp60rest1:
	tstb	%a0@(2)			| null state frame?
	jeq	Lresfp60rest2		| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr	| restore FP control registers
	fmovem	%a0@(FPF_FPSR),%fpsr
	fmovem	%a0@(FPF_FPI),%fpi
	fmovem	%a0@(FPF_REGS),%fp0-%fp7 | restore FP general registers
Lresfp60rest2:
	frestore %a0@			| restore state
	movw	%a1@(PCB_PS),%sr	| no, restore PS
	moveq	#1,%d0			| return 1 (for alternate returns)
	rts
#endif

/*
 * savectx(pcb)
 * Update pcb, saving current processor state.
 */
ENTRY(savectx)
	movl	%sp@(4),%a1
	movw	%sr,%a1@(PCB_PS)
	movl	%usp,%a0		| grab USP
	movl	%a0,%a1@(PCB_USP)	| and save it
	moveml	#0xFCFC,%a1@(PCB_REGS)	| save non-scratch registers

	tstl	_C_LABEL(fputype)	| Do we have FPU?
	jeq	Lsvnofpsave		| No?  Then don't save state.
	lea	%a1@(PCB_FPCTX),%a0	| pointer to FP save area
	fsave	%a0@			| save FP state
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	Lsvsavfp60
#endif  
	tstb	%a0@			| null state frame?
	jeq	Lsvnofpsave		| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS)	| save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,%a0@(FPF_FPCR) | save FP control registers
#if defined(M68060)
	jra	Lsvnofpsave
Lsvsavfp60:
#endif
#endif  
#if defined(M68060)
	tstb	%a0@(2)			| null state frame?
	jeq	Lsvnofpsave		| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS) | save FP general registers
	fmovem	%fpcr,%a0@(FPF_FPCR)	| save FP control registers
	fmovem	%fpsr,%a0@(FPF_FPSR)
	fmovem	%fpi,%a0@(FPF_FPI)
#endif  
Lsvnofpsave:
	moveq	#0,%d0			| return 0
	rts

#if defined(M68040) || defined(M68060)
ENTRY(suline)
	movl	%sp@(4),%a0		| address to write
	movl	_C_LABEL(curpcb),%a1	| current pcb
	movl	#Lslerr,%a1@(PCB_ONFAULT) | where to return to on a fault
	movl	%sp@(8),%a1		| address of line
	movl	%a1@+,%d0		| get lword
	movsl	%d0,%a0@+		| put lword
	nop				| sync
	movl	%a1@+,%d0		| get lword
	movsl	%d0,%a0@+		| put lword
	nop				| sync
	movl	%a1@+,%d0		| get lword
	movsl	%d0,%a0@+		| put lword
	nop				| sync
	movl	%a1@+,%d0		| get lword
	movsl	%d0,%a0@+		| put lword
	nop				| sync
	moveq	#0,%d0			| indicate no fault
	jra	Lsldone
Lslerr:
	moveq	#-1,%d0
Lsldone:
	movl	_C_LABEL(curpcb),%a1	| current pcb
	clrl	%a1@(PCB_ONFAULT)	| clear fault address
	rts
#endif


ENTRY(ecacheon)
	rts

ENTRY(ecacheoff)
	rts

/*
 * Get callers current SP value.
 * Note that simply taking the address of a local variable in a C function
 * doesn't work because callee saved registers may be outside the stack frame
 * defined by A6 (e.g. GCC generated code).
 */
ENTRY_NOPROFILE(getsp)
	movl	%sp,%d0			| get current SP
	addql	#4,%d0			| compensate for return address
	movl	%d0,%a0
	rts

ENTRY_NOPROFILE(getsfc)
	movc	%sfc,%d0
	movl	%d0,%a0
	rts

ENTRY_NOPROFILE(getdfc)
	movc	%dfc,%d0
	movl	%d0,%a0
	rts

/*
 * Load a new user segment table pointer.
 */
ENTRY(loadustp)
	movl	%sp@(4),%d0		| new USTP
	moveq	#PGSHIFT, %d1
	lsll	%d1,%d0			| convert to addr
#if defined(M68040) || defined(M68060)
	cmpl    #MMU_68040,_C_LABEL(mmutype) | 68040?
	jne     LmotommuC               | no, skip
	.word	0xf518			| pflusha
	.long   0x4e7b0806              | movc d0,urp
#ifdef M68060
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jne	Lldno60
	movc	%cacr,%d0
	orl	#IC60_CUBC,%d0		| clear user branch cache entries
	movc	%d0,%cacr
Lldno60:
#endif
	rts
LmotommuC:
#endif
	pflusha				| flush entire TLB
	lea	_C_LABEL(protorp),%a0	| CRP prototype
	movl	%d0,%a0@(4)		| stash USTP
	pmove	%a0@,%crp		| load root pointer
	movl	#CACHE_CLR,%d0
	movc	%d0,%cacr		| invalidate cache(s)
	rts

ENTRY(ploadw)
#ifdef M68030
#if defined(M68040) || defined(M68060)
	cmpl	#MMU_68040,_C_LABEL(mmutype) | 68040?
	jeq	Lploadwskp		| yes, skip
#endif
	movl	%sp@(4),%a0		| address to load
	ploadw	#1,%a0@			| pre-load translation
Lploadwskp:
#endif
	rts

ENTRY(getsr)
	moveq	#0,%d0
	movw	%sr,%d0
	rts

/*
 * _delay(unsigned N)
 *
 * Delay for at least (N/1024) microseconds.
 * This routine depends on the variable:  delay_divisor
 * which should be set based on the CPU clock rate.
 */
ENTRY_NOPROFILE(_delay)
	| d0 = arg = (usecs << 10)
	movl	%sp@(4),%d0
	| d1 = delay_divisor
	movl	_C_LABEL(delay_divisor),%d1
	jra	L_delay			/* Jump into the loop! */

	/*
	 * Align the branch target of the loop to a half-line (8-byte)
	 * boundary to minimize cache effects.  This guarantees both
	 * that there will be no prefetch stalls due to cache line burst
	 * operations and that the loop will run from a single cache
	 * half-line.
	 */
#ifdef __ELF__
	.align	8
#else
	.align	3
#endif
L_delay:
	subl	%d1,%d0
	jgt	L_delay
	rts

/*
 * Save and restore 68881 state.
 */
ENTRY(m68881_save)
	movl	%sp@(4),%a0		| save area pointer
	fsave	%a0@			| save state
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	Lm68060fpsave
#endif
Lm68881fpsave:  
	tstb	%a0@			| null state frame?
	jeq	Lm68881sdone		| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS)	| save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,%a0@(FPF_FPCR) | save FP control registers
Lm68881sdone:
	rts
#endif
#if defined(M68060)
Lm68060fpsave:
	tstb	%a0@(2)			| null state frame?
	jeq	Lm68060sdone		| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS)	| save FP general registers
	fmovem	%fpcr,%a0@(FPF_FPCR)	| save FP control registers
	fmovem	%fpsr,%a0@(FPF_FPSR)           
	fmovem	%fpi,%a0@(FPF_FPI)
Lm68060sdone:   
        rts
#endif  

ENTRY(m68881_restore)
	movl	%sp@(4),%a0		| save area pointer
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	Lm68060fprestore
#endif
Lm68881fprestore:
	tstb	%a0@			| null state frame?
	jeq	Lm68881rdone		| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr/%fpsr/%fpi | restore FP control registers
	fmovem	%a0@(FPF_REGS),%fp0-%fp7	| restore FP general registers
Lm68881rdone:
	frestore %a0@			| restore state
	rts
#endif
#if defined(M68060)
Lm68060fprestore:
	tstb	%a0@(2)			| null state frame?
	jeq	Lm68060fprdone		| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr	| restore FP control registers
	fmovem	%a0@(FPF_FPSR),%fpsr
	fmovem	%a0@(FPF_FPI),%fpi
	fmovem	%a0@(FPF_REGS),%fp0-%fp7 | restore FP general registers
Lm68060fprdone:
	frestore %a0@			| restore state
	rts
#endif

/*
 * Handle the nitty-gritty of rebooting the machine.
 * Basically we just turn off the MMU, restore the Bug's initial VBR
 * and either return to Bug or jump through the ROM reset vector
 * depending on how the system was halted.
 */
ENTRY_NOPROFILE(doboot)
	movw	#PSL_HIGHIPL,%sr
	movl	_C_LABEL(boothowto),%d1	| load howto
	movl	%sp@(4),%d2		| arg
	movl	_ASM_LABEL(bug_vbr),%d3	| Fetch Bug's original VBR value
	movl	_C_LABEL(machineid),%d4	| What type of board is this?
	movl	#CACHE_OFF,%d0
#if defined(M68040) || defined(M68060)
	cmpl	#MMU_68040,_C_LABEL(mmutype)	| 68040/68060?
	jne	Lnocache0		| no, skip
	.word	0xf4f8			| cpusha bc - push and invalidate caches
	nop
	movl	#CACHE40_OFF,%d0
#endif
Lnocache0:
	movc	%d0,%cacr		| disable on-chip cache(s)

#if defined(M68040) || defined(M68060)
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jne	LmotommuF
	movql	#0,%d0
	movc	%d0,%cacr
	.long	0x4e7b0003              | movc d0,tc
	jra	Lbootcommon
LmotommuF:
#endif
	clrl	%sp@-			| value for pmove to TC (turn off MMU)
	pmove	%sp@,%tc		| disable MMU
	addql	#4,%sp

Lbootcommon:
	/*
	 * MMU Switched off by now, so relocate all absolute references
	 */
	ASRELOC(tmpstk, %sp)		| physical SP in case of NMI
	movc	%d3,%vbr		| Restore Bug's VBR
	andl	#RB_SBOOT, %d1		| mask off
	bne	Lsboot			| sboot?
	/* NOT sboot */
	tstl	%d2			| autoboot?
	beq	Ldoreset		| yes!
	CALLBUG(MVMEPROM_EXIT)		| return to bug
	/* NOTREACHED */

Ldoreset:
	movl	#0xff800000,%a0		| Bug's reset vector address
	movl	%a0@+, %a7		| get SP
	movl	%a0@, %a0		| get PC
	jmp	%a0@			| go!

Lsboot: /* sboot */
	tstl	%d2			| autoboot?
	beq	1f			| yes!
	jmp 	0x4000			| back to sboot
1:	jmp	0x400a			| tell sboot to reboot us


/*
 * Misc. global variables.
 */
	.data

GLOBAL(machineid)
	.long	MVME_147	| default to MVME_147

GLOBAL(mmutype)
	.long	MMU_68030	| default to MMU_68030

GLOBAL(cputype)
	.long	CPU_68030	| default to CPU_68030

GLOBAL(fputype)
	.long	FPU_68882	| default to FPU_68882

GLOBAL(protorp)
	.long	0,0		| prototype root pointer

/*
 * Information from first stage boot program
 */
GLOBAL(bootpart)
	.long	0
GLOBAL(bootdevlun)
	.long	0
GLOBAL(bootctrllun)
	.long	0
GLOBAL(bootaddr)
	.long	0
GLOBAL(boothowto)
	.long	0

GLOBAL(want_resched)
	.long	0

GLOBAL(proc0paddr)
	.long	0		| KVA of proc0 u-area

GLOBAL(intiobase)
	.long	0		| KVA of base of internal IO space

GLOBAL(intiolimit)
	.long	0		| KVA of end of internal IO space

GLOBAL(intiobase_phys)
	.long	0		| PA of board's I/O registers

GLOBAL(intiotop_phys)
	.long	0		| PA of top of board's I/O registers

/*
 * interrupt counters (unused due to evcnt(9). Kept here to keep vmstat happy)
 */
GLOBAL(intrnames)
GLOBAL(eintrnames)
GLOBAL(intrcnt)
GLOBAL(eintrcnt)
