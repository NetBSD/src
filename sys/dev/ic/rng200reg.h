/*	$NetBSD: rng200reg.h,v 1.1 2019/09/01 14:44:14 mlelstv Exp $	*/

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael van Elst
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

#ifndef _IC_RNG200REG_H_
#define _IC_RNG200REG_H_

#include <sys/cdefs.h>

/*
 * registers
 */
#define RNG200_CONTROL		0x00
#define   RNG200_RBG_MASK	__BITS(0,12)
#define   RNG200_RBG_ENABLE	__BIT(1)
#define RNG200_RNG_RESET	0x04
#define   RNG_RESET		0x00000001
#define RNG200_RBG_RESET	0x08
#define   RBG_RESET		0x00000001
#define RNG200_STATUS		0x18
#define   RNG200_MASTER_FAIL	__BIT(31)
#define   RNG200_NIST_FAIL	__BIT(5)
#define RNG200_DATA		0x20
#define RNG200_COUNT		0x24
#define   RNG200_COUNT_MASK	__BITS(0,7)

#endif /* !_IC_RNG200REG_H_ */
