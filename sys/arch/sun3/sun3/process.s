
/*
 * The following primitives manipulate the run queues.
 * _whichqs tells which of the 32 queues _qs
 * have processes in them.  Setrq puts processes into queues, Remrq
 * removes them from queues.  The running process is on no queue,
 * other processes are on a queue related to p->p_pri, divided by 4
 * actually to shrink the 0-127 range of priorities into the 32 available
 * queues.
 */

	.globl	_whichqs,_qs,_cnt,_panic
	.globl	_curproc
	.comm	_want_resched,4

/*
 * Setrq(p)
 *
 * Call should be made at spl6(), and p->p_stat should be SRUN
 */
ENTRY(setrq)
	movl	sp@(4),a0
	tstl	a0@(P_RLINK)
	jeq	Lset1
	movl	#Lset2,sp@-
	jbsr	_panic
Lset1:
	clrl	d0
	movb	a0@(P_PRI),d0
	lsrb	#2,d0
	movl	_whichqs,d1
	bset	d0,d1
	movl	d1,_whichqs
	lslb	#3,d0
	addl	#_qs,d0
	movl	d0,a0@(P_LINK)
	movl	d0,a1
	movl	a1@(P_RLINK),a0@(P_RLINK)
	movl	a0,a1@(P_RLINK)
	movl	a0@(P_RLINK),a1
	movl	a0,a1@(P_LINK)
	rts

Lset2:
	.asciz	"setrq"
	.even

/*
 * Remrq(p)
 *
 * Call should be made at spl6().
 */
ENTRY(remrq)
	movl	sp@(4),a0
	clrl	d0
	movb	a0@(P_PRI),d0
	lsrb	#2,d0
	movl	_whichqs,d1
	bclr	d0,d1
	jne	Lrem1
	movl	#Lrem3,sp@-
	jbsr	_panic
Lrem1:
	movl	d1,_whichqs
	movl	a0@(P_LINK),a1
	movl	a0@(P_RLINK),a1@(P_RLINK)
	movl	a0@(P_RLINK),a1
	movl	a0@(P_LINK),a1@(P_LINK)
	movl	#_qs,a1
	movl	d0,d1
	lslb	#3,d1
	addl	d1,a1
	cmpl	a1@(P_LINK),a1
	jeq	Lrem2
	movl	_whichqs,d1
	bset	d0,d1
	movl	d1,_whichqs
Lrem2:
	clrl	a0@(P_RLINK)
	rts

Lrem3:
	.asciz	"remrq"
Lsw0:
	.asciz	"swtch"
	.even

	.globl	_curpcb
	.globl	_masterpaddr	| XXX compatibility (debuggers)
	.data
_masterpaddr:			| XXX compatibility (debuggers)
_curpcb:
	.long	0
pcbflag:
	.byte	0		| copy of pcb_flags low byte
	.align	2
	.comm	nullpcb,SIZEOF_PCB
	.text

/*
 * At exit of a process, do a swtch for the last time.
 * The mapping of the pcb at p->p_addr has already been deleted,
 * and the memory for the pcb+stack has been freed.
 * The ipl is high enough to prevent the memory from being reallocated.
 */
ENTRY(swtch_exit)
	movl	#nullpcb,_curpcb	| save state into garbage pcb
	lea	tmpstk,sp		| goto a tmp stack
	jra	_swtch

/*
 * When no processes are on the runq, Swtch branches to idle
 * to wait for something to come ready.
 */
	.globl	Idle
Lidle:
	stop	#PSL_LOWIPL
Idle:
idle:
	movw	#PSL_HIGHIPL,sr
	tstl	_whichqs
	jeq	Lidle
	movw	#PSL_LOWIPL,sr
	jra	Lsw1

Lbadsw:
	movl	#Lsw0,sp@-
	jbsr	_panic
	/*NOTREACHED*/


.globl _load_u_area;
/*
 * Swtch()
 *
 */
ENTRY(swtch)
	movl	_curpcb,a0		| current pcb
	movw	sr,a0@(PCB_PS)		| save sr before changing ipl
#ifdef notyet
	movl	_curproc,sp@-		| remember last proc running
#endif
	clrl	_curproc
	addql	#1,_cnt+V_SWTCH
Lsw1:
	/*
	 * Find the highest-priority queue that isn't empty,
	 * then take the first proc from that queue.
	 */
	clrl	d0
	lea	_whichqs,a0
	movl	a0@,d1
Lswchk:
	btst	d0,d1
	jne	Lswfnd
	addqb	#1,d0
	cmpb	#32,d0
	jne	Lswchk
	jra	idle
Lswfnd:
	movw	#PSL_HIGHIPL,sr		| lock out interrupts
	movl	a0@,d1			| and check again...
	bclr	d0,d1
	jeq	Lsw1			| proc moved, rescan
	movl	d1,a0@			| update whichqs
	moveq	#1,d1			| double check for higher priority
	lsll	d0,d1			| process (which may have snuck in
	subql	#1,d1			| while we were finding this one)
	andl	a0@,d1
	jeq	Lswok			| no one got in, continue
	movl	a0@,d1
	bset	d0,d1			| otherwise put this one back
	movl	d1,a0@
	jra	Lsw1			| and rescan
Lswok:
	movl	d0,d1
	lslb	#3,d1			| convert queue number to index
	addl	#_qs,d1			| locate queue (q)
	movl	d1,a1
	cmpl	a1@(P_LINK),a1		| anyone on queue?
	jeq	Lbadsw			| no, panic
	movl	a1@(P_LINK),a0			| p = q->p_link
	movl	a0@(P_LINK),a1@(P_LINK)		| q->p_link = p->p_link
	movl	a0@(P_LINK),a1			| q = p->p_link
	movl	a0@(P_RLINK),a1@(P_RLINK)	| q->p_rlink = p->p_rlink
	cmpl	a0@(P_LINK),d1		| anyone left on queue?
	jeq	Lsw2			| no, skip
	movl	_whichqs,d1
	bset	d0,d1			| yes, reset bit
	movl	d1,_whichqs
Lsw2:
	movl	a0,_curproc
	clrl	_want_resched
#ifdef notyet
	movl	sp@+,a1
	cmpl	a0,a1			| switching to same proc?
	jeq	Lswdone			| yes, skip save and restore
#endif
	/*
	 * Save state of previous process in its pcb.
	 */
	movl	_curpcb,a1
	moveml	#0xFCFC,a1@(PCB_REGS)	| save non-scratch registers
	movl	usp,a2			| grab USP (a2 has been saved)
	movl	a2,a1@(PCB_USP)		| and save it
#ifdef FPCOPROC
	lea	a1@(PCB_FPCTX),a2	| pointer to FP save area
	fsave	a2@			| save FP state
	tstb	a2@			| null state frame?
	jeq	Lswnofpsave		| yes, all done
	fmovem	fp0-fp7,a2@(216)	| save FP general registers
	fmovem	fpcr/fpsr/fpi,a2@(312)	| save FP control registers
Lswnofpsave:
#endif

#ifdef DIAGNOSTIC
	tstl	a0@(P_WCHAN)
	jne	Lbadsw
	cmpb	#SRUN,a0@(P_STAT)
	jne	Lbadsw
#endif
	clrl	a0@(P_RLINK)		| clear back link
	movl	a0@(P_ADDR),a1		| get p_addr
	movl	a1,_curpcb
	movb	a1@(PCB_FLAGS+1),pcbflag | copy of pcb_flags low byte
	
	/* see if pmap_activate needs to be called; should remove this */
	movl	a0@(P_VMSPACE),a0	| vmspace = p->p_vmspace
#ifdef DIAGNOSTIC
	tstl	a0			| map == VM_MAP_NULL?
	jeq	Lbadsw			| panic
#endif
	lea	a0@(VM_PMAP),a0		| pmap = &vmspace.vm_pmap
	pea	a1@			| push pcb (at p_addr)
	pea	a0@			| push pmap
	jbsr	_pmap_activate		| pmap_activate(pmap, pcb)
	addql	#8,sp
	movl	_curpcb,a1		| restore p_addr
Lswnochg:
#if 0
/* this stuff was ifdefed out because it can't work under our clock level
 * configuration + it knows about the hp300 clock hardware
 */
#ifdef PROFTIMER
#ifdef notdef
	movw	#SPL6,sr		| protect against clock interrupts
#endif
	bclr	#0,_profon		| clear user profiling bit, was set?
	jeq	Lskipoff		| no, clock off or doing kernel only
#ifdef GPROF
	tstb	_profon			| kernel profiling also enabled?
	jlt	Lskipoff		| yes, nothing more to do
#endif
	CLKADDR(a0)
	movb	#0,a0@(CLKCR2)		| no, just user, select CR3
	movb	#0,a0@(CLKCR3)		| and turn it off
Lskipoff:
#endif /* PROFTIMER */
#endif /* if 0*/
	pea 	a1@
	jbsr    _load_u_area
	addql	#8,sp	
	movl	_curpcb,a1		| restore p_addr
	lea	tmpstk,sp		| now goto a tmp stack for NMI
Lcxswdone:
	moveml	a1@(PCB_REGS),#0xFCFC	| and registers
	movl	a1@(PCB_USP),a0
	movl	a0,usp			| and USP
#if 0
/* profiling support currently borken */
#ifdef PROFTIMER
	tstl	a1@(U_PROFSCALE)	| process being profiled?
	jeq	Lskipon			| no, do nothing
	orb	#1,_profon		| turn on user profiling bit
#ifdef GPROF
	jlt	Lskipon			| already profiling kernel, all done
#endif
	CLKADDR(a0)
	movl	_profint,d1		| profiling interval
	subql	#1,d1			|   adjusted
	movepw	d1,a0@(CLKMSB3)		| set interval
	movb	#0,a0@(CLKCR2)		| select CR3
	movb	#64,a0@(CLKCR3)		| turn it on
Lskipon:
#endif /*proftimer*/
#endif /* if 0 */

#ifdef FPCOPROC
	lea	a1@(PCB_FPCTX),a0	| pointer to FP save area
	tstb	a0@			| null state frame?
	jeq	Lresfprest		| yes, easy
	fmovem	a0@(312),fpcr/fpsr/fpi	| restore FP control registers
	fmovem	a0@(216),fp0-fp7	| restore FP general registers
Lresfprest:
	frestore a0@			| restore state
#endif
	movw	a1@(PCB_PS),sr		| no, restore PS
	moveq	#1,d0			| return 1 (for alternate returns)
	rts

/*
 * savectx(pcb, altreturn)
 * Update pcb, saving current processor state and arranging
 * for alternate return ala longjmp in swtch if altreturn is true.
 */
ENTRY(savectx)
	movl	sp@(4),a1
	movw	sr,a1@(PCB_PS)
	movl	usp,a0			| grab USP
	movl	a0,a1@(PCB_USP)		| and save it
	moveml	#0xFCFC,a1@(PCB_REGS)	| save non-scratch registers
#ifdef FPCOPROC
	lea	a1@(PCB_FPCTX),a0	| pointer to FP save area
	fsave	a0@			| save FP state
	tstb	a0@			| null state frame?
	jeq	Lsvnofpsave		| yes, all done
	fmovem	fp0-fp7,a0@(216)	| save FP general registers
	fmovem	fpcr/fpsr/fpi,a0@(312)	| save FP control registers
Lsvnofpsave:
#endif
	tstl	sp@(8)			| altreturn?
	jeq	Lsavedone
	movl	sp,d0			| relocate current sp relative to a1
	subl	#KSTACK_ADDR,d0		|   (sp is relative to kstack):
	addl	d0,a1			|   a1 += sp - kstack;
	movl	sp@,a1@			| write return pc at (relocated) sp@
Lsavedone:
	moveq	#0,d0			| return 0
	rts

/*
 * update profiling information for the user
 * addupc(pc, &u.u_prof, ticks)
 */
ENTRY(addupc)
	movl	a2,sp@-			| scratch register
	movl	sp@(12),a2		| get &u.u_prof
	movl	sp@(8),d0		| get user pc
	subl	a2@(8),d0		| pc -= pr->pr_off
	jlt	Lauexit			| less than 0, skip it
	movl	a2@(12),d1		| get pr->pr_scale
	lsrl	#1,d0			| pc /= 2
	lsrl	#1,d1			| scale /= 2
	mulul	d1,d0			| pc /= scale
	moveq	#14,d1
	lsrl	d1,d0			| pc >>= 14
	bclr	#0,d0			| pc &= ~1
	cmpl	a2@(4),d0		| too big for buffer?
	jge	Lauexit			| yes, screw it
	addl	a2@,d0			| no, add base
	movl	d0,sp@-			| push address
	jbsr	_fusword		| grab old value
	movl	sp@+,a0			| grab address back
	cmpl	#-1,d0			| access ok
	jeq	Lauerror		| no, skip out
	addw	sp@(18),d0		| add tick to current value
	movl	d0,sp@-			| push value
	movl	a0,sp@-			| push address
	jbsr	_susword		| write back new value
	addql	#8,sp			| pop params
	tstl	d0			| fault?
	jeq	Lauexit			| no, all done
Lauerror:
	clrl	a2@(12)			| clear scale (turn off prof)
Lauexit:
	movl	sp@+,a2			| restore scratch reg
	rts
