/* 	$NetBSD: pstworeg.h,v 1.1 2006/12/02 22:18:47 freza Exp $ */

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

#ifndef _VIRTEX_DEV_PSTWOREG_H_
#define _VIRTEX_DEV_PSTWOREG_H_

/*
 * PeeCee style PS2 serdes core. Doesn't save us much work, really.
 * Each instance handles one PS2 port, the user has to know if it's
 * mouse or keyboard.
 */

#define PSTWO_SIZE 		0x1c

/* Deal with inconsistent notation in Xilinx datasheets. Whee. */
#define PSTWO_BIT(n) 		(0x80000000U >> (n))

#define PSTWO_CTRL 		0x0000 		/* wo */
#define CTRL_RESET 		PSTWO_BIT(7) 	/* Component reset when set */

#define PSTWO_STAT 		0x0004 		/* ro */
#define STAT_TX_BUSY 		PSTWO_BIT(6) 	/* Transmitting byte */
#define STAT_RX_DONE 		PSTWO_BIT(7) 	/* Received a byte */

#define PSTWO_RX_DATA 		0x0008 		/* ro */
#define PSTWO_TX_DATA 		0x000c 		/* wo */
#define DATA_RECV(x) 		((x) >> 24) 	/* RX */
#define DATA_SEND(x) 		((x) << 24) 	/* TX */

#define PSTWO_INTR_STAT 	0x0010 		/* ro */
#define PSTWO_INTR_ACK 		0x0014 		/* wo (reads as STAT) */
#define PSTWO_INTR_MSET 	0x0018 		/* wo (reads as MASK) */
#define PSTWO_INTR_MCLR 	0x001c 		/* wo (reads as MASK) */
#define PSTWO_INTR_MASK 	PSTWO_INTR_MSET

/* PSTWO_INTR_ bits */
#define INTR_RX_FULL 		PSTWO_BIT(2) 	/* Received byte */
#define INTR_RX_ERR 		PSTWO_BIT(3) 	/* Received corrupt packet */
#define INTR_RX_OVER 		PSTWO_BIT(4) 	/* RX overrun */
#define INTR_TX_DONE 		PSTWO_BIT(5) 	/* TX packet acknowledged */
#define INTR_TX_SENT 		PSTWO_BIT(6) 	/* TX packet sent */
#define INTR_TX_TIMO 		PSTWO_BIT(7) 	/* Clock lost while TX */
#define INTR_ANY 		(INTR_RX_FULL | INTR_RX_ERR | INTR_RX_OVER | \
				 INTR_TX_DONE | INTR_TX_SENT | INTR_TX_TIMO)

#endif /*_VIRTEX_DEV_PSTWOREG_H_*/
