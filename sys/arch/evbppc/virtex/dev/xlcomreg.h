/* 	$NetBSD: xlcomreg.h,v 1.1 2006/12/02 22:18:47 freza Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _VIRTEX_DEV_XLCOMREG_H_
#define _VIRTEX_DEV_XLCOMREG_H_

/*
 * Xilinx UART Lite (opb_uartlite_0 in EDK) registers. Note that all
 * line parameter are hardcoded at synthesis time. There is no hardware
 * flow control, just RX and TX signals.
 */

#define XLCOM_SIZE 		0x0c

/* 16B FIFOs */
#define XLCOM_RX_FIFO 		0x0000
#define XLCOM_TX_FIFO 		0x0004

#define XLCOM_STAT 		0x0008 		/* ro */
#define STAT_PARITY_ERR 	0x80
#define STAT_FRAME_ERR 		0x40
#define STAT_OVERRUN_ERR 	0x20
#define STAT_INTR_EN 		0x10 		/* Interrupt enabled */
#define STAT_TX_FULL 		0x08
#define STAT_TX_EMPTY 		0x04
#define STAT_RX_FULL 		0x02
#define STAT_RX_DATA 	 	0x01 		/* RX FIFO has valid data */

#define XLCOM_CNTL 		0x000c 		/* wo */
#define CNTL_INTR_EN 		0x10
#define CNTL_RX_CLEAR 		0x02 		/* Reset/clear FIFOs */
#define CNTL_TX_CLEAR 		0x01

#endif /*_VIRTEX_DEV_XLCOMREG_H_*/
