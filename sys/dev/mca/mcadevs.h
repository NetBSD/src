/*	$NetBSD: mcadevs.h,v 1.5 2001/03/19 22:24:17 jdolecek Exp $	*/

/*
 * THIS FILE AUTOMATICALLY GENERATED.  DO NOT EDIT.
 *
 * generated from:
 *	NetBSD: mcadevs,v 1.4 2001/03/19 22:23:58 jdolecek Exp 
 */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * Copyright (c) 1996-1999 Scott D. Telford.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Scott Telford <s.telford@ed.ac.uk>.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Supported MCA devices
 */

#define	MCA_PRODUCT_AHA1640	0x0F1F	/* Adaptec AHA-1640 SCSI Adapter */
#define	MCA_PRODUCT_3C523	0x6042	/* 3Com EtherLink/MC Ethernet Adapter (3C523) */
#define	MCA_PRODUCT_3C529	0x627C	/* 3Com 3C529 Ethernet Adapter */
#define	MCA_PRODUCT_3C529_TP	0x627D	/* 3Com 3C529-TP Ethernet Adapter */
#define	MCA_PRODUCT_3C529_TM	0x62DB	/* 3Com 3C529 Ethernet Adapter (test mode) */
#define	MCA_PRODUCT_3C529_2T	0x62F6	/* 3Com 3C529 Ethernet Adapter (10base2/T) */
#define	MCA_PRODUCT_3C529_T	0x62F7	/* 3Com 3C529 Ethernet Adapter (10baseT) */
#define	MCA_PRODUCT_ITR	0xE001	/* IBM Token Ring 16/4 Adapter/A */
#define	MCA_PRODUCT_IBM_MOD	0xEDFF	/* IBM Internal Modem */
