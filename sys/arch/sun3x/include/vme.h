/*	$NetBSD: vme.h,v 1.1 1997/10/16 15:39:36 gwr Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * CPU Physical addresses for various VME spaces.
 * The vme unit number selects one of these.
 */

/* vme0 */
#define	VME32D32_BASE	0x80000000
#define	VME32D32_SIZE	(1<<31)
#define	VME32D32_MASK	(VME32D32_SIZE-1)

/* vme1 */
#define	VME24D32_BASE	0x7F000000
#define	VME24D32_SIZE	(1<<24)
#define	VME24D32_MASK	(VME24D32_SIZE-1)

/* vme2 */
#define	VME24D16_BASE	0x7E000000
#define	VME24D16_SIZE	(1<<24)
#define	VME24D16_MASK	(VME24D16_SIZE-1)

/* vme3 */
#define	VME16D32_BASE	0x7D000000
#define	VME16D32_SIZE	(1<<16)
#define	VME16D32_MASK	(VME16D32_SIZE-1)

/* vme4 */
#define	VME16D16_BASE	0x7C000000
#define	VME16D16_SIZE	(1<<16)
#define	VME16D16_MASK	(VME16D16_SIZE-1)

#define VME_UNITS	5
