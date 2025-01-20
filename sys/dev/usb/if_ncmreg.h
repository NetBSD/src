/*	$NetBSD: if_ncmreg.h,v 1.1 2025/01/20 13:54:55 maya Exp $ */

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maya Rashish
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
 * USB Network Control Model (NCM)
 * https://www.usb.org/sites/default/files/NCM10_012011.zip
 */

#ifndef _NCM_H_
#define _NCM_H_

#include <dev/usb/usb.h>

#define NCM_MIN_TX_BUFSZ					\
	(ETHERMTU + ETHER_HDR_LEN				\
	    + sizeof(struct ncm_header16)			\
	    + sizeof(struct ncm_pointer16))

struct ncm_header16 {
#define	NCM_HDR16_SIG	0x484d434e
	uDWord	dwSignature;
	uWord	wHeaderLength;
	uWord	wSequence;
	uWord	wBlockLength;
	uWord	wNdpIndex;
} UPACKED;

struct ncm_header32 {
#define	NCM_HDR32_SIG	0x686d636e
	uDWord	dwSignature;
	uWord	wHeaderLength;
	uWord	wSequence;
	uDWord	dwBlockLength;
	uDWord	dwNdpIndex;
} UPACKED;

struct ncm_pointer16 {
	uWord	wDatagramIndex;
	uWord	wDatagramLength;
} UPACKED;

struct ncm_dptab16 {
#define	NCM_PTR16_SIG_NO_CRC	0x304d434e
#define	NCM_PTR16_SIG_CRC	0x314d434e
	uDWord			dwSignature;
	uWord			wLength;
	uWord			wNextNdpIndex;
	struct ncm_pointer16	datagram[2];
} UPACKED;

struct ncm_pointer32 {
	uDWord	dwDatagramIndex;
	uDWord	dwDatagramLength;
} UPACKED;

struct ncm_dptab32 {
#define	NCM_PTR32_SIG_NO_CRC	0x306d636e
#define	NCM_PTR32_SIG_CRC	0x316d636e
	uDWord			dwSignature;
	uWord			wLength;
	uWord			wReserved6;
	uWord			wNextNdpIndex;
	uDWord			dwReserved12;
	struct ncm_pointer32	datagram[2];
} UPACKED;

#define NCM_GET_NTB_PARAMETERS 0x80
struct ncm_ntb_parameters {
	uWord	wLength;
	uByte	bmNtbFormatsSupported[2];
	uDWord	dwNtbInMaxSize;
	uWord	wNdpInDivisor;
	uWord	wNdpInPayloadRemainder;
	uWord	wNdpInAlignment;
	uWord	wReserved;
	uDWord	dwNtbOutMaxSize;
	uWord	wNdpOutDivisor;
	uWord	wNdpOutPayloadRemainder;
	uWord	wNdpOutAlignment;
	uWord	wNtbOutMaxDatagrams;
} UPACKED;

#endif /* _NCM_H_ */
