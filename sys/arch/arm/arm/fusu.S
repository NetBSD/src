/*	$NetBSD: fusu.S,v 1.6 2019/04/07 04:11:56 thorpej Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996-1998 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_multiprocessor.h"
#include "opt_cpuoptions.h"

#include "assym.h"

#include <machine/asm.h>

#include <arm/locore.h>

#define	RETURN_SUCCESS							\
	mov	r0, #0x00000000					;	\
	mov	pc, lr

#define	RETURN_ERROR_ALREADY_IN_R0					\
	mov	pc, lr

#define	UFETCHSTORE_PROLOGUE						\
	GET_CURPCB(r2)						;	\
	adr	r3, _C_LABEL(ufetchstore_fault)			;	\
	str	r3, [r2, #PCB_ONFAULT]

#define	UFETCHSTORE_EPILOGUE						\
	mov	r3, #0x00000000					;	\
	str	r3, [r2, #PCB_ONFAULT]


/* LINTSTUB: int _ufetch_8(const uint8_t *uaddr, uint8_t *valp); */

ENTRY(_ufetch_8)
	UFETCHSTORE_PROLOGUE

	ldrbt	r3, [r0]
	strb	r3, [r1]

	UFETCHSTORE_EPILOGUE
	RETURN_SUCCESS
END(_ufetch_8)

#if (ARM_ARCH_2 + ARM_ARCH_3 + ARM_ARCH_4 + ARM_ARCH_5) > 0
/* LINTSTUB: int _ufetch_16_armv2(const uint16_t *uaddr, uint16_t *valp); */
ENTRY(_ufetch_16_armv2)
	UFETCHSTORE_PROLOGUE

	ldrbt	r3, [r0], #1
	strb	r3, [r1], #1

	ldrbt	r3, [r0]
	strb	r3, [r1]

	UFETCHSTORE_EPILOGUE
	RETURN_SUCCESS
END(_ufetch_16_armv2)

/* LINTSTUB: int _ufetch_16(const uint16_t *uaddr, uint16_t *valp); */
STRONG_ALIAS(_ufetch_16,_ufetch_16_armv2)

#else /* XXX */

/* LINTSTUB: int _ufetch_16_armv6(const uint16_t *uaddr, uint16_t *valp); */
ENTRY(_ufetch_16_armv6)
	UFETCHSTORE_PROLOGUE

	ldrht	r3, [r0]
	strh	r3, [r1]

	UFETCHSTORE_EPILOGUE
	RETURN_SUCCESS
END(_ufetch_16_armv6)

/* LINTSTUB: int _ufetch_16(const uint16_t *uaddr, uint16_t *valp); */
STRONG_ALIAS(_ufetch_16,_ufetch_16_armv6)
#endif /* (ARM_ARCH_2 + ARM_ARCH_3 + ARM_ARCH_4 + ARM_ARCH_5) > 0 */

/* LINTSTUB: int _ufetch_32(const uint32_t *uaddr, uint32_t *valp); */
ENTRY(_ufetch_32)
	UFETCHSTORE_PROLOGUE

	ldrt	r3, [r0]
	str	r3, [r1]

	UFETCHSTORE_EPILOGUE
	RETURN_SUCCESS
END(_ufetch_32)

/* LINTSTUB: int _ustore_8(uint8_t *uaddr, uint8_t val); */
ENTRY(_ustore_8)
	UFETCHSTORE_PROLOGUE

	strbt	r1, [r0]

	UFETCHSTORE_EPILOGUE
	RETURN_SUCCESS
END(_ustore_8)

#if (ARM_ARCH_2 + ARM_ARCH_3 + ARM_ARCH_4 + ARM_ARCH_5) > 0
/* LINTSTUB: int _ustore_16_armv2(uint16_t *uaddr, uint16_t val); */
ENTRY(_ustore_16_armv2)
	UFETCHSTORE_PROLOGUE

#ifdef __ARMEB__
	mov	r3, r1, lsr #8
	strbt	r3, [r0], #1
#else
	strbt	r1, [r0], #1
	mov	r1, r1, lsr #8
#endif
	strbt	r1, [r0]

	UFETCHSTORE_EPILOGUE
	RETURN_SUCCESS
END(_ustore_16_armv2)

/* LINTSTUB: int _ustore_16(uint16_t *uaddr, uint16_t val); */
STRONG_ALIAS(_ustore_16,_ustore_16_armv2)

#else /* XXX */

/* LINTSTUB: int _ustore_16_armv4(uint16_t *uaddr, uint16_t val); */
ENTRY(_ustore_16_armv6)
	UFETCHSTORE_PROLOGUE

	strht	r1, [r0]

	UFETCHSTORE_EPILOGUE
	RETURN_SUCCESS
END(_ustore_16_armv6)

/* LINTSTUB: int _ustore_16(uint16_t *uaddr, uint16_t val); */
STRONG_ALIAS(_ustore_16,_ustore_16_armv6)
#endif /* (ARM_ARCH_2 + ARM_ARCH_3 + ARM_ARCH_4 + ARM_ARCH_5) > 0 */

/* LINTSTUB: int _ustore_32(uint32_t *uaddr, uint32_t val); */
ENTRY(_ustore_32)
	UFETCHSTORE_PROLOGUE

	strt	r1, [r0]

	UFETCHSTORE_EPILOGUE
	RETURN_SUCCESS
END(_ustore_32)

ENTRY_NP(ufetchstore_fault)
	UFETCHSTORE_EPILOGUE
	RETURN_ERROR_ALREADY_IN_R0
END(ufetchstore_fault)

/* XXXJRT Can we G/C this? */
	.data
	.align	0
	.global _C_LABEL(block_userspace_access)
_C_LABEL(block_userspace_access):
	.word	0
