/*	$NetBSD: locore.S,v 1.1 2003/08/19 10:55:00 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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

#include <machine/asm.h>
#include <machine/io.h>
#include <machine/intr.h>

#define	HALT(x)	jrst %4,x

#define	PGSZ	01000	/* In words */
#define	PGMSK	(PGSZ-1)
#define	NPGMSK	0777000	/* (~PGMSK) */
#define	SP	%17
#define	FP	%16

#define	PTOFF(off,arg) skipa %2,.+1 ; arg ; movem %2,off(%1)

#define	INTRDEF(eptoff, p0off, dst) \
	PTOFF(eptoff,xpcw p0off) ; xmovei %3,dst ; movem %3,@[.long p0off+3]
/*
 * A bunch of instructions. These should be recognized by the assembler.
 */
#define	xpcw jrst %7,
#define	xjen jrst %6,

/*
 * Usage of page 0 for interrupt dispatchers.
 */
#define	HIGHPOS	020
#define	DDBPOS	024
#define	CLKPOS	030
#define	NETPOS	034
#define	BIOPOS	040
#define	TTYPOS	044
#define	SNETPOS	050
#define	SCLKPOS	054

		.data
		.globl avail_start	/* Physical memory availability */
		.globl avail_end
		.globl proc0paddr,ept
avail_start:	.long 0
avail_end:	.long 0
proc0paddr:	.long 0
istack:		.long 0
ept:		.long 0
#ifdef DDB
ddbstack:	.long 0
#endif

/* Longwords to switch between register blocks quick */
moniw:		.long 0400100000000	/* Current block 0, previous user */
useriw:		.long 0401100000000	/* Current user, previous user */
intiw:		.long 0402000000000	/* Current int, previous monitor */
ddbiw:		.long 0403000000000	/* Current ddb, previous monitor */

ENTRY(start)

	/*
	 * Set interrupt stack directly after end,
	 * and at least one page in size.
	 */
	xmovei %10,end
	movem %10,istack
	addi %10,2*PGSZ
	trz %10,PGMSK		# trunc to page boundary
	datao %PAG,intiw	# switch to interrupt block
	move %17,istack		# set interrupt stack
	datao %PAG,moniw	# back to monitor block

	/*
	 * Setup Executive Process Table as next.
	 */
	movem %10,ept
	move %1,010
	skipa %2,.+1
	pushj %17,earov
	movem %2,0421(%1)
	PTOFF(0422,jrst esov)
	PTOFF(0423,jrst etrap3)

	/*
	 * Interrupt setup. This is somewhat hairy, the interrupt protocol
	 * on KL10 is not simple. Page 0 in memory is used for xpcw.
	 */
	INTRDEF(0514,CLKPOS,_hardclock)
	INTRDEF(040,HIGHPOS,panic)	/* Yech! */
	INTRDEF(042,DDBPOS,_ddbintr)
/*	INTRDEF(044,CLKPOS,_hardclock) separate vector */
	INTRDEF(046,NETPOS,_netintr)
	INTRDEF(050,BIOPOS,_biointr)
	INTRDEF(052,TTYPOS,_ttyintr)
	INTRDEF(054,SNETPOS,_softnet)
	INTRDEF(056,SCLKPOS,_softclock)

	addi %10,PGSZ
	/*
	 * Process 0 user area (proc0paddr) located directly thereafter.
	 * Guard pages?
	 */
	movem %10,proc0paddr
	addi %10,PGSZ		/* Skip UPT */
	move %17,%10		/* Process 0 stackpointer XXX uarea */
	addi %10,PGSZ		/* The rest of USPACE */
	move %1,proc0paddr
	pushj %17,setup_upt	/* Set trap addresses */

#ifdef DDB
	movem %10,ddbstack	# ddb has a private stack
	addi %10,PGSZ		# make it one page in size
	movsi %1,0403000		# set ac block, current == 3
	datao %PAG,1		# doit!
	move %17,ddbstack	# put stack address in stack pointer
	datao %PAG,moniw		# back to monitor ac block,
#endif

	movem %10,avail_start	/* available memory */

	/*
	 * Setup memory management.
	 */
	pushj %17,pmap_bootstrap

	/*
	 * Done here, jump to main now.
	 */
	pushj %17,main
	HALT(1)

/*
 * Check if this address is nonexisting.
 */
ENTRY(badaddr)
	move %3,proc0paddr+0503	# save old trap pc
	xmovei %4,bad1		# address if trapping
	movem %4,proc0paddr+0503	# set new address
	move %2,020(%1)		# skip register block
	movem %3,proc0paddr+0503	# restore trap handler
	movei %1,0		# not a bad address
	popj %17,

bad1:	movem %3,proc0paddr+0503	# restore trap handler
	movei %1,1		# was bad address
	popj %17,

/*
 * Copy routines.
 */
	.data
reta:	.long 0		# return address after fault
retv:	.long 0		# return value

ENTRY(copyin) HALT(017)
ENTRY(copyout) HALT(022)
ENTRY(copyinstr) HALT(023)
ENTRY(copyoutstr) HALT(024)

/*
 * Copy string in-kernel.
 * Check for fault in trap routine, not here.
 * XXX - *done length is wrong if it faults, is this OK?
 */
ENTRY(copystr)
jrst %4,0567
	xmovei %6,L_cps3	# r6 = &L_cps3;
	movem %6,reta	# reta = r6
	setzm retv	# retv = 0;
	move %6,%3	# olen = len

L_cps2:	subi %3,1	# len--
	camn %3,[.long -1] # if (len == -1)
	jrst L_cps1	#	goto L_cps1
	ldb %7,%1		# r7 = *from
	dpb %7,%2		# *to = r7
	ibp %1		# from++;
	ibp %2		# to++;
	jumpn %7,L_cps2	# if (r7 != 0) goto L_cps2

L_cps1:	jumpe %4,L_cps3	# if (done == 0) goto L_cps3
	sub %6,%3		# olen -= len;
	addi %6,1	# olen++;
	movem %6,(%4)	# *done = olen;

L_cps3:	move %1,retv	# r1 = retv
	popj %17,	# return r1;



ENTRY(kcopy) HALT(026)

ENTRY(suword) HALT(027)
ENTRY(subyte) HALT(030)
ENTRY(suswintr) HALT(031)
ENTRY(fuswintr) HALT(032)

/*
 * spl routines should be macron in the future.
 */
#define SVPRI coni PI,%1 ; andi %1,PI_ALL
#define ALTERSPL(val) SVPRI ; cono PI,(PI_LVLOFF|val) ; popj SP,

ENTRY(splhigh)
	ALTERSPL(PI_ALL)	# Block all levels

ENTRY(splclock)
	ALTERSPL(PI_CLOCK)	# Block hardclock

ENTRY(splnet)
	ALTERSPL(PI_NET)	# Block network adapters

ENTRY(splsched)
	ALTERSPL(PI_ALL)	# Block all levels XXX

ENTRY(splvm)
	ALTERSPL(PI_BIO|PI_TTY|PI_NET)	# Block disks, ttys and network adapters

ENTRY(spllock)
	ALTERSPL(PI_ALL)	# Block all levels XXX

ENTRY(splbio)
	ALTERSPL(PI_BIO)	# Block disks

ENTRY(splstatclock)
	ALTERSPL(PI_CLOCK)	# Block hardclock

ENTRY(splsoftnet)
	ALTERSPL(PI_SOFTCLOCK)	# Block soft network protocols

ENTRY(spltty)
	ALTERSPL(PI_TTY)	# Block ttys

ENTRY(spl0)		# Unblock everything
	cono PI,PI_LVLON|PI_ON|PI_ALL
	popj SP,


/*
 * This routine is problematic. The only way to lower an interrupt is
 * through xjen, which resets the interrupt level. For now we don't try
 * to do anything clever; set CLKF_BASEPRI() to 0 and avoid this routine.
 */
ENTRY(spllowersoftclock)
	HALT(044)

/*
 * Lower the current interrupt level. If softclock was blocked but is
 * now unblocked, call for an interrupt.
 */
ENTRY(splx)
	move %1,-1(SP)
	iori %1,PI_LVLON
	cono PI,(%1)
	skipn %1,L_want_soft	# Skip if any softbit is set
	 popj SP,
	trne %1,PI_SOFTCLOCK	# Skip if softclock not set.
	 pushj SP,setsoftclock	# Try to send another
	trne %1,PI_SOFTNET	# Skip if softnet not set.
	 pushj SP,setsoftnet	# Try to send another setsoftnet
	popj SP,

	.data
L_want_soft:
	.long 0

/*
 * Ask for a softclock interrupt. If softclock is blocked, set a flag that
 * posts a softclock interrupt when it is unblocked.
 */
ENTRY(setsoftclock)
	coni PI,%1		# get current interrupt mask
	trne %1,PI_SOFTCLOCK	# skip if masked
	 cono PI,PI_INIT|PI_SOFTCLOCK	# send interrupt request
	movei %1,PI_SOFTCLOCK
	iorm %1,L_want_soft	# request soft intr
	popj SP,

ENTRY(setsoftnet)
	coni PI,%1		# get current interrupt mask
	trne %1,PI_SOFTNET	# skip if masked
	 cono PI,PI_INIT|PI_SOFTNET	# send interrupt request
	movei %1,PI_SOFTNET
	iorm %1,L_want_soft	# request soft intr
	popj SP,

ENTRY(sigcode)
	HALT(0)
ENTRY(esigcode)

ENTRY(signotify)
	popj %17,

ENTRY(need_proftick) HALT(066)

earov:
	
	HALT(067)
esov: HALT(070)
etrap3: HALT(071)

/*
 * Interrupt handling stuff
 */
#define	INTR_SAVE(myword) \
	datai PAG,myword ; datao PAG,intiw ; \
	xmovei SP,011(SP) ; dmovem %0,-010(SP) ; dmovem %2,-6(SP) ; \
	dmovem %4,-4(SP) ; dmovem %6,-2(SP) ; move %1,myword ; movem %1,(SP)
#define INTR_RESTORE(myword) \
	move %1,(SP) ; tlz %1,0100000 ; movem %1,myword ; dmove %6,-2(SP) ; \
	dmove %4,-4(SP) ; dmove %2,-6(SP) ; dmove %0,-010(SP) ; \
	xmovei %17,-011(SP) ; datao %PAG,myword ;

	.data
L_hcpw:	.long 0			/* Saved pager word at hardclock */
L_scpw:	.long 0			/* Saved pager word at softclock */
L_biow:	.long 0			/* Saved pager word at diskintr */
L_ddbw:	.long 0			/* Saved pager word at ddb */

	.text

#ifdef DDB
# Workaround for buggy assembler
xct1:	blt %1,ddb_regs+15
xct2:	blt %1,015
#endif

/*
 * Save a trapframe onto the interrupt stack with the current
 * register contents. First set the previous context correct.
 */
_ddbintr:
#ifdef DDB
	datai %PAG,L_ddbw ; datao %PAG,ddbiw
	hllz %1,L_ddbw		# fetch only left-half of pager word
	tlz %1,0770777		# clear everything except "current" AC block
	lsh %1,-3		# put ac block in previous position
	ior %1,ddbiw		# create pager word
	datao %PAG,01		# set correct previous context

	movei %1,ddb_regs	# only lower 18 bits please
	xct %1,xct1		# pxct, fetch regs
	move %2,@[ .long DDBPOS+1 ] # get pc
	xmovei %1,ddb_regs
	movem %2,020(%1)	# store pc

	pushj %17,ddbintr

	movsi %1,ddb_regs
	xct %4,xct2		# restore regs
	xmovei %1,ddb_regs
	move %2,020(%1)
	movem %2,@[ .long DDBPOS+1 ]

#endif
	cono PI,020100		# clear ddb interrupt
	xjen @[.long DDBPOS]

_hardclock:
	cono TIM,061750		# Clear timer interrupt
	INTR_SAVE(L_hcpw)
	xmovei %1,(SP)		# Clock frame
	pushj SP,hardclock
	INTR_RESTORE(L_hcpw)
	xjen @[.long CLKPOS]

_netintr:
	HALT(072)

_biointr:
#if 1
	INTR_SAVE(L_biow)
	pushj SP,rhintr
	INTR_RESTORE(L_biow)
	xjen @[.long BIOPOS]
#else
	HALT(073)
#endif

_ttyintr:
	HALT(074)

_softnet:
	HALT(075)

_softclock:
	INTR_SAVE(L_scpw)
	pushj SP,softclock
	INTR_RESTORE(L_scpw)
	cono PI,PI_DROP|PI_SOFTCLOCK	# clear softclock interrupt
	movei %1,2
	andi %1,L_want_soft		# clear request word
	xjen @[.long SCLKPOS]


ENTRY(pcb_ovtrap) HALT(0421)
ENTRY(pcb_ovstack) HALT(0422)
ENTRY(pcb_trap3) HALT(0423)
ENTRY(pcb_knotrap) HALT(0430)
ENTRY(pcb_ktrap) HALT(0431)
ENTRY(pcb_snotrap) HALT(0432)
ENTRY(pcb_strap) HALT(0433)
ENTRY(pcb_cnotrap) HALT(0434)
ENTRY(pcb_ctrap) HALT(0435)
ENTRY(pcb_pnotrap) HALT(0436)
ENTRY(pcb_ptrap) HALT(0437)

ENTRY(pcb_pfnpc)
	push SP,proc0paddr
	pushj SP,pagefail
	HALT(0503);

/*
 * Write the needed trap addresses into a new User Process Table.
 * The UPT page must be zeroed before call. Pointer to it in reg 1.
 */
ENTRY(setup_upt)
	PTOFF(0421, jrst pcb_ovtrap)
	PTOFF(0422, jrst pcb_ovstack)
	PTOFF(0423, jrst pcb_trap3)
	PTOFF(0430, .long pcb_knotrap)
	PTOFF(0431, .long pcb_ktrap)
	PTOFF(0432, .long pcb_snotrap)
	PTOFF(0433, .long pcb_strap)
	PTOFF(0434, .long pcb_cnotrap)
	PTOFF(0435, .long pcb_ctrap)
	PTOFF(0436, .long pcb_pnotrap)
	PTOFF(0437, .long pcb_ptrap)
	PTOFF(0503, .long pcb_pfnpc)
	popj SP,

#ifdef __PCC__
ENTRY(datao)
	move %1,-1(%17)			# get device
	lsh %1,030			# shift left 24 step
	ior %1,[.long 0700140000002 ]	# create instruction
	move %2,-2(%17)			# get value
	xct %1				# doit!
	popj %17,

ENTRY(datai)
	move %1,-1(%17)			# get device
	lsh %1,030			# shift left 24 step
	ior %1,[.long 0700040000001 ]	# create instruction
	xct %1				# doit!
	popj %17,

ENTRY(cono)
	move %1,-1(%17)			# get device
	lsh %1,030			# shift left 24 step
	tlo %1,0700200			# create instruction
	hrr %1,-2(%17)			# get value
	xct %1				# doit!
	popj %17,
	
ENTRY(coni)
	move %1,-1(%17)			# get device
	lsh %1,030			# shift left 24 step
	ior %1,[.long 0700240000001 ]	# create instruction
	xct %1				# doit!
	popj %17,

ENTRY(blko)
	HALT(0604);

ENTRY(blki)
	move %1,-1(%17)			# get device
	lsh %1,030			# shift left 24 step
	ior %1,[.long 0700000000001 ]	# create instruction
	xct %1
	popj %17,

ENTRY(clrpt)
	HALT(0606);
#endif

#ifdef DDB
ENTRY(setjmp)
	move %1,-1(%17)		# Get jmp_buf pointer
	move %2,(%17)		# Get return PC
	movem %2,1(%1)		# Save PC
	move %2,%17		# Get SP
	movem %2,2(%1)		# Save SP
	movem %16,3(%1)		# Save FP
	dmovem %10,010(%1)	# Save reg 10+11
	dmovem %12,012(%1)	# Save reg 12+13
	dmovem %14,014(%1)	# Save reg 14+15
	setz %1,		# Return 0
	popj %17,

ENTRY(longjmp)
	move %3,-1(%17)		# Get jmp_buf pointer
	move %1,-2(%17)		# Get return value
	dmove %14,014(%3)	# Restore reg 14+15
	dmove %12,012(%3)	# Restore reg 12+13
	dmove %10,010(%3)	# Restore reg 10+11
	move %16,3(%3)		# Restore FP
	move %17,2(%3)		# Restore SP
	move %2,1(%3)		# Get return PC
	movem %2,(%17)		# Save PC
	popj %17,

ENTRY(cpu_Debugger)
	cono PI,06300		# debugger runs on IPL 1
jrst %4,.+1
	popj %17,
#endif
