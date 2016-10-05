/*	$NetBSD: support.S,v 1.6.36.1 2016/10/05 20:55:29 skrll Exp $	*/

/*-
 * Copyright (c) 1998 Doug Rabson
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/*-
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <machine/asm.h>
#include <machine/ia64_cpu.h>

#include "assym.h"

.text

/*
 * ia64_change_mode:	change mode to/from physical mode
 *
 * Arguments:
 *	r14	psr for desired mode
 *
 * Modifies:
 *	r15-r19	scratch
 *	ar.bsp	tranlated to new mode
 */
ENTRY_NOPROFILE(ia64_change_mode, 0)
	rsm	psr.i | psr.ic
	mov	r19=ar.rsc		// save rsc while we change mode
	tbit.nz	p6,p7=r14,17		// physical or virtual ?
	;;
	mov	ar.rsc=0		// turn off RSE
(p6)	mov	r15=7			// RR base for virtual addresses
(p7)	mov	r15=0			// RR base for physical addresses
	flushrs				// no dirty registers please
	srlz.i
	;;
	mov	r16=ar.bsp
	mov	r17=rp
	mov	r18=ar.rnat
	;;
	dep	r16=r15,r16,61,3	// new address of ar.bsp
	dep	r17=r15,r17,61,3	// new address of rp
	dep	sp=r15,sp,61,3		// new address of sp
	;;
	mov	ar.bspstore=r16
	mov	rp=r17
	;;
1:	mov	r16=ip
	mov	ar.rnat=r18
	mov	cr.ipsr=r14		// psr for new mode
	;;
	add	r16=2f-1b,r16		// address to rfi to
	;;
	dep	r16=r15,r16,61,3	// new mode address for rfi
	;;
	mov	cr.iip=r16		// setup for rfi
	mov	cr.ifs=r0
	;;
	rfi

2:	mov	ar.rsc=r19		// restore ar.rsc
	br.ret.sptk.few rp		// now in new mode
END(ia64_change_mode)

/*
 * ia64_physical_mode:	change mode to physical mode
 *
 * Return:
 *	ret0	psr to restore
 *
 * Modifies:
 *	r15-r18	scratch
 *	ar.bsp	tranlated to physical mode
 *	psr.i	cleared
 */
ENTRY(ia64_physical_mode, 0)
	mov	r14=psr
	mov	ret0=psr
	movl	r15=(IA64_PSR_I|IA64_PSR_IT|IA64_PSR_DT|IA64_PSR_RT|IA64_PSR_DFL|IA64_PSR_DFH)
	movl	r16=IA64_PSR_BN
	;;
	andcm	r14=r14,r15		// clear various xT bits
	;;
	or	r14=r14,r16		// make sure BN=1
	or	ret0=ret0,r16		// make sure BN=1

	br.cond.sptk.many ia64_change_mode
END(ia64_physical_mode)

/*
 * ia64_call_efi_physical:	call an EFI procedure in physical mode
 *
 * Arguments:
 *	in0		Address of EFI procedure descriptor
 *	in1-in5		Arguments to EFI procedure
 *
 * Return:
 *	ret0-ret3	return values from EFI
 *
 */
ENTRY(ia64_call_efi_physical, 6)
	.prologue
	.regstk	6,4,5,0
	.save	ar.pfs,loc0
	alloc	loc0=ar.pfs,6,4,5,0
	;;
	.save	rp,loc1
	mov	loc1=rp
	;;
	.body
	br.call.sptk.many rp=ia64_physical_mode
	;;
	mov	loc2=r8			// psr to restore mode
	mov	loc3=gp			// save kernel gp
	ld8	r14=[in0],8		// function address
	;;
	mov	out0=in1
	mov	out1=in2
	mov	out2=in3
	mov	out3=in4
	mov	out4=in5
	ld8	gp=[in0]		// function gp value
	;;
	mov	b6=r14
	;;
	br.call.sptk.many rp=b6		// call EFI procedure
	mov	gp=loc3			// restore kernel gp
	;;
	mov	r14=loc2		// psr to restore mode
	br.call.sptk.many rp=ia64_change_mode
	;;
	mov	rp=loc1
	mov	ar.pfs=loc0
	;;
	br.ret.sptk.many rp
END(ia64_call_efi_physical)

/**************************************************************************/

ENTRY(fusufault, 0)
{	.mib
	st8.rel		[r15]=r0		// Clear onfault.
	add		ret0=-1,r0
	br.ret.sptk	rp
	;;
}
END(fusufault)

/*
 * casuptr(intptr_t *p, intptr_t old, intptr_t new)
 *	Perform a compare-exchange in user space.
 */
ENTRY(casuptr, 3)
{	.mlx
	add		r15=PC_CURLWP,r13
	movl		r14=VM_MAX_ADDRESS
	;;
}
{	.mib
	ld8		r15=[r15]		// r15 = curthread
	cmp.geu		p6,p0=in0,r14
(p6)	br.dpnt.few	1f
	;;
}
{	.mlx
	add		r15=L_PCB,r15
	movl		r14=fusufault
	;;
}
{	.mmi
	ld8		r15=[r15]		// r15 = PCB
	;;
	mov		ar.ccv=in1
	add		r15=PCB_ONFAULT,r15
	;;
}
{	.mmi
	st8		[r15]=r14		// Set onfault
	;;
	cmpxchg8.rel	ret0=[in0],in2,ar.ccv
	nop		0
	;;
}
{	.mfb
	st8.rel		[r15]=r0		// Clear onfault
	nop		0
	br.ret.sptk	rp
	;;
}
1:
{	.mfb
	add		ret0=-1,r0
	nop		0
	br.ret.sptk	rp
	;;
}
END(casuptr)

/*
 * subyte(void *addr, int byte)
 * suword16(void *addr, int word)
 * suword32(void *addr, int word)
 * suword64|suword(void *addr, long word)
 *	Store in user space
 */

ENTRY(subyte, 2)
{	.mlx
	add		r15=PC_CURLWP,r13
	movl		r14=VM_MAX_ADDRESS
	;;
}
{	.mib
	ld8		r15=[r15]		// r15 = curthread
	cmp.geu		p6,p0=in0,r14
(p6)	br.dpnt.few	1f
	;;
}
{	.mlx
	add		r15=L_PCB,r15
	movl		r14=fusufault
	;;
}
{	.mmi
	ld8		r15=[r15]		// r15 = PCB
	;;
	nop		0
	add		r15=PCB_ONFAULT,r15
	;;
}
{	.mmi
	st8		[r15]=r14		// Set onfault
	;;
	st1.rel		[in0]=in1
	nop		0
	;;
}
{	.mib
	st8.rel		[r15]=r0		// Clear onfault
	mov		ret0=r0
	br.ret.sptk	rp
	;;
}
1:
{	.mfb
	add		ret0=-1,r0
	nop		0
	br.ret.sptk	rp
	;;
}
END(subyte)

ENTRY(suword16, 2)
{	.mlx
	add		r15=PC_CURLWP,r13
	movl		r14=VM_MAX_ADDRESS
	;;
}
{	.mib
	ld8		r15=[r15]		// r15 = curthread
	cmp.geu		p6,p0=in0,r14
(p6)	br.dpnt.few	1f
	;;
}
{	.mlx
	add		r15=L_PCB,r15
	movl		r14=fusufault
	;;
}
{	.mmi
	ld8		r15=[r15]		// r15 = PCB
	;;
	nop		0
	add		r15=PCB_ONFAULT,r15
	;;
}
{	.mmi
	st8		[r15]=r14		// Set onfault
	;;
	st2.rel		[in0]=in1
	nop		0
	;;
}
{	.mib
	st8.rel		[r15]=r0		// Clear onfault
	mov		ret0=r0
	br.ret.sptk	rp
	;;
}
1:
{	.mfb
	add		ret0=-1,r0
	nop		0
	br.ret.sptk	rp
	;;
}
END(suword16)

ENTRY(suword32, 2)
{	.mlx
	add		r15=PC_CURLWP,r13
	movl		r14=VM_MAX_ADDRESS
	;;
}
{	.mib
	ld8		r15=[r15]		// r15 = curthread
	cmp.geu		p6,p0=in0,r14
(p6)	br.dpnt.few	1f
	;;
}
{	.mlx
	add		r15=L_PCB,r15
	movl		r14=fusufault
	;;
}
{	.mmi
	ld8		r15=[r15]		// r15 = PCB
	;;
	nop		0
	add		r15=PCB_ONFAULT,r15
	;;
}
{	.mmi
	st8		[r15]=r14		// Set onfault
	;;
	st4.rel		[in0]=in1
	nop		0
	;;
}
{	.mib
	st8.rel		[r15]=r0		// Clear onfault
	mov		ret0=r0
	br.ret.sptk	rp
	;;
}
1:
{	.mfb
	add		ret0=-1,r0
	nop		0
	br.ret.sptk	rp
	;;
}
END(suword32)

ENTRY(suword64, 2)
XENTRY(suword)
{	.mlx
	add		r15=PC_CURLWP,r13
	movl		r14=VM_MAX_ADDRESS
	;;
}
{	.mib
	ld8		r15=[r15]		// r15 = curthread
	cmp.geu		p6,p0=in0,r14
(p6)	br.dpnt.few	1f
	;;
}
{	.mlx
	add		r15=L_PCB,r15
	movl		r14=fusufault
	;;
}
{	.mmi
	ld8		r15=[r15]		// r15 = PCB
	;;
	nop		0
	add		r15=PCB_ONFAULT,r15
	;;
}
{	.mmi
	st8		[r15]=r14		// Set onfault
	;;
	st8.rel		[in0]=in1
	nop		0
	;;
}
{	.mib
	st8.rel		[r15]=r0		// Clear onfault
	mov		ret0=r0
	br.ret.sptk	rp
	;;
}
1:
{	.mfb
	add		ret0=-1,r0
	nop		0
	br.ret.sptk	rp
	;;
}
END(suword64)

/*
 * fubyte(void *addr, int byte)
 * fuword16(void *addr, int word)
 * fuword32(void *addr, int word)
 * fuword64|fuword(void *addr, long word)
 *	Fetch from user space
 */

ENTRY(fubyte, 1)
{	.mlx
	add		r15=PC_CURLWP,r13
	movl		r14=VM_MAX_ADDRESS
	;;
}
{	.mib
	ld8		r15=[r15]		// r15 = curthread
	cmp.geu		p6,p0=in0,r14
(p6)	br.dpnt.few	1f
	;;
}
{	.mlx
	add		r15=L_PCB,r15
	movl		r14=fusufault
	;;
}
{	.mmi
	ld8		r15=[r15]		// r15 = PCB
	;;
	nop		0
	add		r15=PCB_ONFAULT,r15
	;;
}
{	.mmi
	st8		[r15]=r14		// Set onfault
	;;
	mf
	nop		0
	;;
}
{	.mmb
	ld1		ret0=[in0]
	st8.rel		[r15]=r0		// Clear onfault
	br.ret.sptk	rp
	;;
}
1:
{	.mfb
	add		ret0=-1,r0
	nop		0
	br.ret.sptk	rp
	;;
}
END(fubyte)

ENTRY(fuword16, 2)
{	.mlx
	add		r15=PC_CURLWP,r13
	movl		r14=VM_MAX_ADDRESS
	;;
}
{	.mib
	ld8		r15=[r15]		// r15 = curthread
	cmp.geu		p6,p0=in0,r14
(p6)	br.dpnt.few	1f
	;;
}
{	.mlx
	add		r15=L_PCB,r15
	movl		r14=fusufault
	;;
}
{	.mmi
	ld8		r15=[r15]		// r15 = PCB
	;;
	nop		0
	add		r15=PCB_ONFAULT,r15
	;;
}
{	.mmi
	st8		[r15]=r14		// Set onfault
	;;
	mf
	nop		0
	;;
}
{	.mmb
	ld2		ret0=[in0]
	st8.rel		[r15]=r0		// Clear onfault
	br.ret.sptk	rp
	;;
}
1:
{	.mfb
	add		ret0=-1,r0
	nop		0
	br.ret.sptk	rp
	;;
}
END(fuword16)

ENTRY(fuword32, 2)
{	.mlx
	add		r15=PC_CURLWP,r13
	movl		r14=VM_MAX_ADDRESS
	;;
}
{	.mib
	ld8		r15=[r15]		// r15 = curthread
	cmp.geu		p6,p0=in0,r14
(p6)	br.dpnt.few	1f
	;;
}
{	.mlx
	add		r15=L_PCB,r15
	movl		r14=fusufault
	;;
}
{	.mmi
	ld8		r15=[r15]		// r15 = PCB
	;;
	nop		0
	add		r15=PCB_ONFAULT,r15
	;;
}
{	.mmi
	st8		[r15]=r14		// Set onfault
	;;
	mf
	nop		0
	;;
}
{	.mmb
	ld4		ret0=[in0]
	st8.rel		[r15]=r0		// Clear onfault
	br.ret.sptk	rp
	;;
}
1:
{	.mfb
	add		ret0=-1,r0
	nop		0
	br.ret.sptk	rp
	;;
}
END(fuword32)

ENTRY(fuword64, 2)
XENTRY(fuword)
{	.mlx
	add		r15=PC_CURLWP,r13
	movl		r14=VM_MAX_ADDRESS
	;;
}
{	.mib
	ld8		r15=[r15]		// r15 = curthread
	cmp.geu		p6,p0=in0,r14
(p6)	br.dpnt.few	1f
	;;
}
{	.mlx
	add		r15=L_PCB,r15
	movl		r14=fusufault
	;;
}
{	.mmi
	ld8		r15=[r15]		// r15 = PCB
	;;
	nop		0
	add		r15=PCB_ONFAULT,r15
	;;
}
{	.mmi
	st8		[r15]=r14		// Set onfault
	;;
	mf
	nop		0
	;;
}
{	.mmb
	ld8		ret0=[in0]
	st8.rel		[r15]=r0		// Clear onfault
	br.ret.sptk	rp
	;;
}
1:
{	.mfb
	add		ret0=-1,r0
	nop		0
	br.ret.sptk	rp
	;;
}
END(fuword64)

/*
 * fuswintr(void *addr)
 * suswintr(void *addr)
 */

ENTRY(fuswintr, 1)
{	.mfb
	add		ret0=-1,r0
	nop		0
	br.ret.sptk	rp
	;;
}
END(fuswintr)

ENTRY(suswintr, 0)
{	.mfb
	add		ret0=-1,r0
	nop		0
	br.ret.sptk	rp
	;;
}
END(suswintr)

/**************************************************************************/

/*
 * Copy a null-terminated string within the kernel's address space.
 * If lenp is not NULL, store the number of chars copied in *lenp
 *
 * int copystr(char *from, char *to, size_t len, size_t *lenp);
 */
ENTRY(copystr, 4)
	mov	r14=in2			// r14 = i = len
	cmp.eq	p6,p0=r0,in2
(p6)	br.cond.spnt.few 2f		// if (len == 0), bail out

1:	ld1	r15=[in0],1		// read one byte
	;;
	st1	[in1]=r15,1		// write that byte
	add	in2=-1,in2		// len--
	;;
	cmp.eq	p6,p0=r0,r15
	cmp.ne	p7,p0=r0,in2
	;;
(p6)	br.cond.spnt.few 2f		// if (*from == 0), bail out
(p7)	br.cond.sptk.few 1b		// if (len != 0) copy more

2:	cmp.eq	p6,p0=r0,in3
(p6)	br.cond.dpnt.few 3f		// if (lenp != NULL)
	sub	r14=r14,in2		// *lenp = (i - len)
	;;
	st8	[in3]=r14

3:	cmp.eq	p6,p0=r0,r15
(p6)	br.cond.spnt.few 4f		// *from == '\0'; leave quietly

	mov	ret0=ENAMETOOLONG	// *from != '\0'; error.
	br.ret.sptk.few rp

4:	mov	ret0=0			// return 0.
	br.ret.sptk.few rp
END(copystr)

ENTRY(copyinstr, 4)
	.prologue
	.regstk	4, 3, 4, 0
	.save	ar.pfs,loc0
	alloc	loc0=ar.pfs,4,3,4,0
	.save	rp,loc1
	mov	loc1=rp
	.body

	movl	loc2=VM_MAX_ADDRESS		// make sure that src addr
	;;
	cmp.geu	p6,p0=in0,loc2			// is in user space.
	;;
(p6)	br.cond.spnt.few copyefault		// if it's not, error out.
	movl	r14=copyerr			// set up fault handler.
	add	r15=PC_CURLWP,r13		// find curthread
	;;
	ld8	r15=[r15]
	;;
	add	r15=L_PCB,r15			// find pcb
	;;
	ld8	r15=[r15]
	;;
	add	loc2=PCB_ONFAULT,r15
	;;
	st8	[loc2]=r14
	;;
	mov	out0=in0
	mov	out1=in1
	mov	out2=in2
	mov	out3=in3
	;;
	br.call.sptk.few rp=copystr		// do the copy.
	st8	[loc2]=r0			// kill the fault handler.
	mov	ar.pfs=loc0			// restore ar.pfs
	mov	rp=loc1				// restore ra.
	br.ret.sptk.few rp			// ret0 left over from copystr
END(copyinstr)

ENTRY(copyoutstr, 4)
	.prologue
	.regstk	4, 3, 4, 0
	.save	ar.pfs,loc0
	alloc	loc0=ar.pfs,4,3,4,0
	.save	rp,loc1
	mov	loc1=rp
	.body

	movl	loc2=VM_MAX_ADDRESS		// make sure that dest addr
	;;
	cmp.geu	p6,p0=in1,loc2			// is in user space.
	;;
(p6)	br.cond.spnt.few copyefault		// if it's not, error out.
	movl	r14=copyerr			// set up fault handler.
	add	r15=PC_CURLWP,r13		// find curthread
	;;
	ld8	r15=[r15]
	;;
	add	r15=L_PCB,r15			// find pcb
	;;
	ld8	r15=[r15]
	;;
	add	loc2=PCB_ONFAULT,r15
	;;
	st8	[loc2]=r14
	;;
	mov	out0=in0
	mov	out1=in1
	mov	out2=in2
	mov	out3=in3
	;;
	br.call.sptk.few rp=copystr		// do the copy.
	st8	[loc2]=r0			// kill the fault handler.
	mov	ar.pfs=loc0			// restore ar.pfs
	mov	rp=loc1				// restore ra.
	br.ret.sptk.few rp			// ret0 left over from copystr
END(copyoutstr)

/*
 * int kcopy(const void *from, void *to, size_t len);
 * Copy len bytes, abort on fault.
 */

ENTRY(kcopy, 3)
	.prologue
	.regstk	3, 3, 3, 0
	.save	ar.pfs,loc0
	alloc	loc0=ar.pfs,3,3,3,0
	.save	rp,loc1
	mov	loc1=rp
	.body

	movl	r14=copyerr			// set up fault handler.
	add	r15=PC_CURLWP,r13		// find curthread
	;;
	ld8	r15=[r15]
	;;
	add	r15=L_PCB,r15			// find pcb
	;;
	ld8	r15=[r15]
	;;
	add	loc2=PCB_ONFAULT,r15
	;;
	st8	[loc2]=r14
	;;
	mov	out0=in0
	mov	out1=in1
	mov	out2=in2
	mov	ret0=r0				// XXX netbsd kcopy same as freebsd?
	;;
	br.call.sptk.few rp=bcopy		// do the copy.
	st8	[loc2]=r0			// kill the fault handler.
	mov	ar.pfs=loc0			// restore ar.pfs
	mov	rp=loc1				// restore ra.
	br.ret.sptk.few rp
END(kcopy)

ENTRY(copyin, 3)
	.prologue
	.regstk	3, 3, 3, 0
	.save	ar.pfs,loc0
	alloc	loc0=ar.pfs,3,3,3,0
	.save	rp,loc1
	mov	loc1=rp
	.body

	movl	loc2=VM_MAX_ADDRESS		// make sure that src addr
	;;
	cmp.geu	p6,p0=in0,loc2			// is in user space.
	;;
(p6)	br.cond.spnt.few copyefault		// if it's not, error out.
	movl	r14=copyerr			// set up fault handler.
	add	r15=PC_CURLWP,r13		// find curthread
	;;
	ld8	r15=[r15]
	;;
	add	r15=L_PCB,r15			// find pcb
	;;
	ld8	r15=[r15]
	;;
	add	loc2=PCB_ONFAULT,r15
	;;
	st8	[loc2]=r14
	;;
	mov	out0=in0
	mov	out1=in1
	mov	out2=in2
	mov	ret0=r0				// return zero for copy{in,out}
	;;
	br.call.sptk.few rp=bcopy		// do the copy.
	st8	[loc2]=r0			// kill the fault handler.
	mov	ar.pfs=loc0			// restore ar.pfs
	mov	rp=loc1				// restore ra.
	br.ret.sptk.few rp
END(copyin)

ENTRY(copyout, 3)
	.prologue
	.regstk	3, 3, 3, 0
	.save	ar.pfs,loc0
	alloc	loc0=ar.pfs,3,3,3,0
	.save	rp,loc1
	mov	loc1=rp
	.body

	movl	loc2=VM_MAX_ADDRESS		// make sure that dest addr
	;;
	cmp.geu	p6,p0=in1,loc2			// is in user space.
	;;
(p6)	br.cond.spnt.few copyefault		// if it's not, error out.
	movl	r14=copyerr			// set up fault handler.
	add	r15=PC_CURLWP,r13		// find curthread
	;;
	ld8	r15=[r15]
	;;
	add	r15=L_PCB,r15			// find pcb
	;;
	ld8	r15=[r15]
	;;
	add	loc2=PCB_ONFAULT,r15
	;;
	st8	[loc2]=r14
	;;
	mov	out0=in0
	mov	out1=in1
	mov	out2=in2
	mov	ret0=r0				// return zero for copy{in,out}
	;;
	br.call.sptk.few rp=bcopy		// do the copy.
	st8	[loc2]=r0			// kill the fault handler.
	mov	ar.pfs=loc0			// restore ar.pfs
	mov	rp=loc1				// restore ra.
	br.ret.sptk.few rp
END(copyout)

ENTRY(copyerr, 0)
	add	r14=PC_CURLWP,r13 ;;		// find curthread
	ld8	r14=[r14] ;;
	add	r14=L_PCB,r14 ;;		// curthread->td_addr
	ld8	r14=[r14] ;;
	add	r14=PCB_ONFAULT,r14 ;;		// &curthread->td_pcb->pcb_onfault
	st8	[r14]=r0			// reset fault handler

	br.ret.sptk.few rp
END(copyerr)

ENTRY(copyefault, 0)
	add	r14=PC_CURLWP,r13 ;;		// find curthread
	ld8	r14=[r14] ;;
	add	r14=L_PCB,r14 ;;		// curthread->td_addr
	ld8	r14=[r14] ;;
	add	r14=PCB_ONFAULT,r14 ;;		// &curthread->td_pcb->pcb_onfault
	st8	[r14]=r0			// reset fault handler

	mov	ret0=EFAULT			// return EFAULT
	br.ret.sptk.few rp
END(copyefault)

#if defined(GPROF)
/*
 * Important registers:
 *      r8      structure return address
 *      rp      our return address
 *      in0     caller's ar.pfs
 *      in1     caller's gp
 *      in2     caller's rp
 *      in3     GOT entry
 *      ar.pfs  our pfs
 */
ENTRY_NOPROFILE(_mcount, 4)
	alloc		loc0 = ar.pfs, 4, 3, 2, 0
	mov		loc1 = r8
	mov		loc2 = rp
	;;
	mov		out0 = in2
	mov		out1 = rp
	br.call.sptk	rp = __mcount
	;;
1:
	mov		gp = in1
	mov		r14 = ip
	mov		b7 = loc2
	;;
	add		r14 = 2f - 1b, r14
	mov		ar.pfs = loc0
	mov		rp = in2
	;;
	mov		b7 = r14
	mov		b6 = loc2
	mov		r8 = loc1
	mov		r14 = in0
	br.ret.sptk	b7
	;;
2:
	mov		ar.pfs = r14
	br.sptk		b6
	;;
END(_mcount)
#endif
