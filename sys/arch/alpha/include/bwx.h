/* $NetBSD: bwx.h,v 1.6.34.1 2012/04/17 00:05:55 yamt Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#ifndef _ALPHA_BWX_H_
#define	_ALPHA_BWX_H_

/*
 * Alpha Byte/Word Extension instructions.
 *
 * These instructions are available on EV56 (21164A) and later processors.
 *
 * See "Alpha Architecture Handbook, Version 3", DEC order number EC-QD2KB-TE.
 */

static __inline uint8_t
alpha_ldbu(volatile uint8_t *a0)
{
	uint8_t v0;

	__asm volatile("ldbu %0, %1"
		: "=r" (v0)
		: "m" (*a0));

	return (v0);
}

static __inline uint16_t
alpha_ldwu(volatile uint16_t *a0)
{
	uint16_t v0;

	__asm volatile("ldwu %0, %1"
		: "=r" (v0)
		: "m" (*a0));

	return (v0);
}

static __inline void
alpha_stb(volatile uint8_t *a0, uint8_t a1)
{

	__asm volatile("stb %1, %0"
		: "=m" (*a0)
		: "r" (a1));
}

static __inline void
alpha_stw(volatile uint16_t *a0, uint16_t a1)
{

	__asm volatile("stw %1, %0"
		: "=m" (*a0)
		: "r" (a1));
}

static __inline uint8_t
alpha_sextb(uint8_t a0)
{
	uint8_t v0;

	__asm volatile("sextb %1, %0"
		: "=r" (v0)
		: "r" (a0));

	return (v0);
}

static __inline uint16_t
alpha_sextw(uint16_t a0)
{
	uint16_t v0;

	__asm volatile("sextw %1, %0"
		: "=r" (v0)
		: "r" (a0));

	return (v0);
}

#endif /* _ALPHA_BWX_H_ */
