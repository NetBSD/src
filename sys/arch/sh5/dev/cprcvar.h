/*	$NetBSD: cprcvar.h,v 1.1 2002/07/05 13:31:51 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_CPRCVAR_H
#define _SH5_CPRCVAR_H

/*
 * The following structure describes the clock rates of the
 * various clock domains provided by the CPRC module. Clock
 * rates are expressed in Hz values.
 *
 * Other drivers in the system can refer to this to determine
 * their own clock rates as appropriate.
 *
 * XXX: Will need to change the "u_int" to "u_long" should
 * technology improve to the point where we have > 4GHz cpus :)
 */
struct cprc_clocks {
	u_int	cc_cpu;		/* CPU Clock */
	u_int	cc_emi;		/* External Memory Interface Clock */
	u_int	cc_superhyway;	/* SuperHyway Clock */
	u_int	cc_peripheral;	/* Peripheral Bus Clock */
	u_int	cc_pci;		/* PCI Bus Clock */
	u_int	cc_femi;	/* Flash External Memory Interface Clock */
	u_int	cc_stbus;	/* ST Legacy Bus Clock */
};

extern struct cprc_clocks cprc_clocks;

#endif /* _SH5_CPRCVAR_H */
