/*	$NetBSD: e32boot.cpp,v 1.1 2013/04/28 12:11:27 kiyohara Exp $	*/
/*
 * Copyright (c) 2012, 2013 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <e32base.h>
#include <e32def.h>
#include <e32std.h>

#include "cpu.h"
#include "e32boot.h"
#include "ekern.h"
#include "epoc32.h"
#include "netbsd.h"


class E32BootLDD : public DLogicalDevice {
public:
	E32BootLDD(void);
	virtual TInt Install(void);
	virtual void GetCaps(TDes8 &) const;
	virtual DLogicalChannel *CreateL(void);
};

class E32BootChannel : public DLogicalChannel {
public:
	E32BootChannel(DLogicalDevice *);

protected:
	virtual void DoCancel(TInt);
	virtual void DoRequest(TInt, TAny *, TAny *);
	virtual TInt DoControl(TInt, TAny *, TAny *);

private:
	EPOC32 *epoc32;
	TAny *safeAddress;

	TInt BootNetBSD(NetBSD *, struct btinfo_common *);
};


/* E32Dll() function is required by all DLLs. */
GLDEF_C TInt
E32Dll(TDllReason)
{

	return KErrNone;
}

EXPORT_C DLogicalDevice *
CreateLogicalDevice(void)
{

	return new E32BootLDD;
}

E32BootLDD::E32BootLDD(void)
{
	/* Nothing */
}

TInt
E32BootLDD::Install(void)
{

	return SetName(&E32BootName);
}

void
E32BootLDD::GetCaps(TDes8 &aDes) const
{
	TVersion version(0, 0, 0);	/* XXXXX: What is it? Don't check? */

	aDes.FillZ(aDes.MaxLength());
	aDes.Copy((TUint8 *)&version, Min(aDes.MaxLength(), sizeof(version)));
}

DLogicalChannel *
E32BootLDD::CreateL(void)
{

	return new (ELeave) E32BootChannel(this);
}


E32BootChannel::E32BootChannel(DLogicalDevice *aDevice)
    : DLogicalChannel(aDevice)
{

	epoc32 = new EPOC32;
	safeAddress = NULL;
}

void
E32BootChannel::DoCancel(TInt aReqNo)
{
	/* Nothing */
}

void
E32BootChannel::DoRequest(TInt aReqNo, TAny *a1, TAny *a2)
{
	/* Nothing */
}

TInt
E32BootChannel::DoControl(TInt aFunction, TAny *a1, TAny *a2)
{

	switch (aFunction) {
	case KE32BootGetProcessorID:
	{
		TInt id;

		__asm("mrc	p15, 0, %0, c0, c0" : "=r"(id));
		*(TUint *)a1 = id;
		break;
	}

	case KE32BootSetSafeAddress:
	{
		safeAddress = (TAny *)PAGE_ALIGN(a1);
		break;
	}

	case KE32BootBootNetBSD:
	{
		NetBSD *netbsd = (NetBSD *)a1;
		struct btinfo_common *bootinfo = (struct btinfo_common *)a2;

		BootNetBSD(netbsd, bootinfo);

		/* NOTREACHED */

		break;
	}

	default:
		break;
	}
	return KErrNone;
}

TInt
E32BootChannel::BootNetBSD(NetBSD *netbsd, struct btinfo_common *bootinfo)
{
	TAny *mmu_disabled, *ttb;

	__asm("adr %0, mmu_disabled" : "=r"(mmu_disabled));
	mmu_disabled = epoc32->GetPhysicalAddress(mmu_disabled);
	/*
	 * ARMv3 can't read TTB from CP15 C1.
	 * Also can't read Control Register.
	 */
	ttb = epoc32->GetPhysicalAddress(epoc32->GetTTB());

	__asm __volatile("				\
	mrs	r12, cpsr;				\
	/* Clear PSR_MODE and Interrupts */		\
	bic	r12, r12, #0xdf;			\
	/* Disable Interrupts(IRQ/FIQ) */		\
	orr	r12, r12, #(3 << 6);			\
	/* Set SVC32 MODE */				\
	orr	r12, r12, #0x13;			\
	msr	cpsr_c, r12;				\
							\
	ldr	r10, [%0, #0x0];			\
	ldr	sp, [%0, #0x4];				\
	ldr	lr, [%0, #0x8];				\
	mov	r12, %1;				\
	" :: "r"(netbsd), "r"(bootinfo));

	__asm __volatile("				\
	mov	r7, %2;					\
	mov	r8, %1;					\
	mov	r9, %0;					\
							\
	/* Set all domains to 15 */			\
	mov	r0, #0xffffffff;			\
	mcr	p15, 0, r0, c3, c0;			\
							\
	/* Disable MMU */				\
	mov	r0, #0x38; /* WBUF | 32BP | 32BD */	\
	mcr	p15, 0, r0, c1, c0, 0;			\
							\
	mov	pc, r7;					\
							\
mmu_disabled:						\
	/*						\
	 * r8	safe address(maybe frame-buffer address)\
	 * r9	ttb					\
	 * r10	buffer (netbsd)				\
	 * r11	memory descriptor			\
	 * r12	bootinfo				\
	 * sp	load descriptor				\
	 * lr	entry point				\
	 */						\
	/* save lr to r7 before call functions. */	\
	mov	r7, lr;					\
							\
	mov	r0, r8;					\
	mov	r1, r9;					\
	bl	vtop;					\
	mov	r8, r0;					\
							\
	/*						\
	 * Copy bootinfo to safe address.		\
	 * That addr used to framebuffer by EPOC32.	\
	 */						\
	mov	r0, r12;				\
	mov	r1, r9;					\
	bl	vtop;					\
	mov	r1, r0;					\
	mov	r12, r8;				\
	mov	r0, r8;					\
	mov	r2, #0x400;				\
	bl	copy;					\
							\
	/* save lr(r7) to r8. it is no need. */		\
	mov	r8, r7;					\
							\
	/* Copy loader to safe address + 0x400. */	\
	add	r0, r12, #0x400;			\
	adr	r1, miniloader_start;			\
	adr	r2, miniloader_end;			\
	sub	r2, r2, r1;				\
	bl	copy;					\
							\
	/* Make load-descriptor to safe addr + 0x800. */\
	mov	r0, sp;					\
	mov	r1, r9;					\
	bl	vtop;					\
	mov	sp, r0;					\
	add	r4, r12, #0x800;			\
							\
next_section:						\
	ldmia	sp!, {r5 - r7};				\
							\
next_page:						\
	add	r0, r10, r6;				\
	mov	r1, r9;					\
	bl	vtop;					\
	/* vtop returns set mask to r2 */		\
	orr	r2, r0, r2;				\
	add	r2, r2, #1;				\
	sub	r2, r2,	r0;				\
	cmp	r2, r7;					\
	movgt	r2, r7;					\
	mov	r1, r0;					\
	mov	r0, r5;					\
	stmia	r4!, {r0 - r2};				\
	add	r5, r5, r2;				\
	add	r6, r6, r2;				\
	subs	r7, r7, r2;				\
	bgt	next_page;				\
							\
	ldr	r0, [sp];				\
	cmp	r0, #0xffffffff;			\
	beq	fin;					\
	/* Pad to section align. */			\
	str	r5, [r4], #4;				\
	mov	r6, #0xffffffff;			\
	str	r6, [r4], #4;				\
	sub	r2, r0, r5;				\
	str	r2, [r4], #4;				\
	b	next_section;				\
							\
fin:							\
	stmia	r4, {r5 - r7};				\
	add	sp, r12, #0x800;			\
							\
	/* save lr(r8) to r11. r11 is no need. */	\
	mov	r11, r8;				\
							\
	/* Fixup load-descriptor by BTINFO_MEMORY. */	\
	mov	r10, r12;				\
	mov	r9, sp;					\
	add	r8, sp, #12;				\
next_bootinfo:						\
	ldmia	r10, {r0, r1};				\
	cmp	r1, #0;		/* BTINFO_NONE */	\
	beq	btinfo_none;				\
							\
	cmp	r1, #2;		/* BTINFO_MEMORY */	\
	beq	btinfo_memory;				\
	add	r10, r10, r0;				\
	b	next_bootinfo;				\
							\
btinfo_none:						\
	/* ENOMEM */					\
	add	r2, r12, #0x800;			\
	mov	r1, #640;				\
	mov	r0, #0x00ff0000;			\
	orr	r0, r0, #0x00ff;			\
98:							\
	str	r0, [r2], #4;				\
	subs	r1, r1, #4;				\
	bgt	98b;					\
	mov	r1, #640;				\
	mov	r0, #0xff000000;			\
	orr	r0, r0, #0xff00;			\
99:							\
	str	r0, [r2], #4;				\
	subs	r1, r1, #4;				\
	bgt	99b;					\
100:							\
	b	100b;					\
							\
btinfo_memory:						\
	ldmia	r10!, {r4 - r7};			\
	ldr	r4, [r9, #0];				\
	subs	r4, r4, r6;				\
	addgt	r6, r6, r4;				\
	subgt	r7, r7, r4;				\
next_desc:						\
	ldmia	r9, {r3 - r5};				\
	ldmia	r8!, {r0 - r2};				\
	add	r3, r3, r5;				\
	add	r4, r4, r5;				\
	cmp	r3, r0;					\
	cmpeq	r4, r1;					\
	beq	join_desc;				\
							\
	ldr	r3, [r9, #0];				\
	cmp	r3, r6;					\
	strlt	r6, [r9, #0];	/* Fixup */		\
	cmp	r5, r7;					\
	bgt	split_desc;				\
	add	r6, r6, r5;				\
	sub	r7, r7, r5;				\
	add	r9, r9, #12;				\
	stmia	r9, {r0 - r2};				\
	cmp	r0, #0xffffffff;			\
	beq	fixuped;				\
	b	next_desc;				\
							\
join_desc:	/* Join r8 descriptor to r9. */		\
	add	r5, r5, r2;				\
	str	r5, [r9, #8];				\
	b	next_desc;				\
							\
split_desc:	/* Split r9 descriptor. */		\
	ldr	r3, [r9, #0];				\
	ldr	r4, [r9, #4];				\
	str	r7, [r9, #8];				\
							\
	sub	r6, r5, r7;				\
	add	r5, r4, r7;				\
	add	r4, r3, r7;				\
	sub	r8, r8, #12;	/* Back to prev desc */	\
	add	r9, r9, #12;/* Point to splited desc */ \
							\
	cmp	r8, r9;					\
	bne	2f;					\
	add	r0, r8, #12;				\
	mov	r1, r8;					\
	mov	r2, #0;					\
1:							\
	ldr	r3, [r8, r2];				\
	add	r2, r2, #12;				\
	cmp	r3, #0xffffffff;			\
	bne	1b;					\
	bl	copy;					\
	add	r8, r8, #12; /* Point to moved desc */	\
2:							\
	stmia	r9, {r4 - r6};				\
	b	next_bootinfo;				\
							\
fixuped:						\
	/* Jump to miniloader. Our LR is entry-point! */\
	add	pc, r12, #0x400;			\
							\
vtop:							\
	/*						\
	 * paddr vtop(vaddr, ttb)			\
	 */						\
	bic	r2, r0, #0x000f0000; /* L1_ADDR_BITS */	\
	bic	r2, r2, #0x0000ff00; /* L1_ADDR_BITS */	\
	bic	r2, r2, #0x000000ff; /* L1_ADDR_BITS */	\
	mov	r2, r2, lsr #(20 - 2);			\
	ldr	r1, [r1, r2];				\
	and	r3, r1, #0x3;	/* L1_TYPE_MASK */	\
	cmp	r3, #0;					\
	bne	valid;					\
							\
invalid:						\
	mov	r0, #-1;				\
	mov	pc, lr;					\
							\
valid:							\
	cmp	r3, #0x2;				\
	bgt	3f;					\
	beq	2f;					\
							\
1:	/* Coarse L2 */					\
	mov	r2, #10;				\
	b	l2;					\
							\
2:	/* Section */					\
	mov	r2, #0xff000000;    /* L1_S_ADDR_MASK */\
	add	r2, r2, #0x00f00000;/* L1_S_ADDR_MASK */\
	mvn	r2, r2;					\
	bic	r3, r1, r2;				\
	and	r0, r0, r2;				\
	orr	r0, r3, r0;				\
	mov	pc, lr;					\
							\
3:	/* Fine L2 */					\
	mov	r2, #12;				\
l2:							\
	mov	r3, #1;					\
	mov	r3, r3, lsl r2;				\
	sub	r3, r3, #1;				\
	bic	r1, r1, r3;	/* L2 table */		\
	mov	r3, r0, lsl #12;			\
	mov	r3, r3, lsr #12;			\
	sub	r2, r2, #22;				\
	rsb	r2, r2, #0;				\
	mov	r3, r3, lsr r2;	/* index for L2 */	\
	ldr	r1, [r1, r3, lsl #2];			\
	and	r3, r1, #0x3;	/* L2_TYPE_MASK */	\
	cmp	r3, #0;					\
	beq	invalid;				\
	cmp	r3, #2;					\
	movlt	r2, #16;	/* L2_L_SHIFT */	\
	moveq	r2, #12;	/* L2_S_SHIFT */	\
	movgt	r2, #10;	/* L2_T_SHIFT */	\
	mov	r3, #1;					\
	mov	r2, r3, lsl r2;				\
	sub	r2, r2, #1;				\
	bic	r3, r1, r2;				\
	and	r0, r0, r2;				\
	orr	r0, r3, r0;				\
	mov	pc, lr;					\
							\
miniloader_start:					\
	b	miniloader;				\
							\
copy:							\
	/*						\
	 * void copy(dest, src, len)			\
	 */						\
	cmp	r0, r1;					\
	bgt	rcopy;					\
lcopy:							\
	ldr	r3, [r1], #4;				\
	str	r3, [r0], #4;				\
	subs	r2, r2, #4;				\
	bgt	lcopy;					\
	mov	pc, lr;					\
rcopy:							\
	subs	r2, r2, #4;				\
	ldr	r3, [r1, r2];				\
	str	r3, [r0, r2];				\
	bgt	rcopy;					\
	mov	pc, lr;					\
							\
							\
miniloader:						\
	/*						\
	 * r11	entry-point				\
	 * r12	bootinfo				\
	 * sp	load-descriptor				\
	 */						\
load:							\
	ldmia	sp!, {r0 - r2};				\
	cmp	r0, #0xffffffff;			\
	beq	end;					\
	cmp	r1, #0xffffffff;/* Skip section align */\
	blne	copy;		 /* or copy */		\
	b	load;					\
							\
end:							\
	mov	r0, r12;				\
	mov	pc, r11;				\
	nop;						\
	nop;						\
miniloader_end:						\
							\
	"
	::"r"(ttb),
	  "r"(safeAddress),
	  "r"(mmu_disabled)
	);

	/* NOTREACHED */

	return -1;
}
