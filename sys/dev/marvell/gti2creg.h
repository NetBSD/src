/*	$NetBSD: gti2creg.h,v 1.3.6.1 2006/04/22 11:39:09 simonb Exp $	*/

/*
 * Copyright (c) 2005 Brocade Communcations, inc.
 * All rights reserved.
 *
 * Written by Matt Thomas for Brocade Communcations, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Brocade Communications, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BROCADE COMMUNICATIONS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL EITHER BROCADE COMMUNICATIONS, INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_MARVELL_GTI2CREG_H_
#define	_DEV_MARVELL_GTI2CREG_H_

#define	I2C_REG_SlaveAddr	0xc000
#define	I2C_REG_ExtSlaveAddr	0xc010
#define	I2C_REG_Data		0xc004
#define	I2C_REG_Control		0xc008
#define	I2C_REG_Status		0xc00c
#define	I2C_REG_BaudRate	0xc00c
#define	I2C_REG_SoftReset	0xc01c

#define	I2C_SlaveAddr_GCE	0x0001	/* Act as Slave */
#define	I2C_SlaveAddr_SAddr	0x7E

#define	I2C_Control_ACK		0x04
#define	I2C_Control_IFlg	0x08
#define	I2C_Control_Stop	0x10
#define	I2C_Control_Start	0x20
#define	I2C_Control_TWSIEn	0x40
#define	I2C_Control_IntEn	0x80

/*
 * F(I2C) = F(Tclk) / ( 10 * (M + 1) * (2^(N+1)))
 * For Tclk = 100 MHz, M =  4, N = 4: F = 62.5 kHz
 * For Tclk = 100 MHz, M = 13, N = 3: F = 96.2 kHz
 */
#define	I2C_BaudRate(M, N)	(((M) << 3) | (N))
#define	I2C_BaudRate_62_5K	I2C_BaudRate(4, 4)
#define	I2C_BaudRate_96_2K	I2C_BaudRate(13, 3)

#define	I2C_Status_BusError	0x00	/* Bus error */
#define I2C_Status_Started	0x08	/* Start condition xmitted */
#define	I2C_Status_ReStarted	0x10	/* Repeated start condition xmitted */
#define	I2C_Status_AddrWriteAck	0x18	/* Adr + wr bit xmtd, ack rcvd */
#define	I2C_Status_AddrWriteNoAck 0x20	/* Adr + wr bit xmtd, NO ack rcvd */
#define	I2C_Status_MasterWriteAck 0x28	/* Master xmtd data byte, ack rcvd */
#define	I2C_Status_MasterWriteNoAck 0x30 /* Master xmtd data byte, NO ack rcvd*/
#define	I2C_Status_MasterLostArb 0x38	/* Master lost arbitration during
					   address or data transfer */
#define	I2C_Status_AddrReadAck	0x40	/* Adr + rd bit xmtd, ack rcvd */
#define	I2C_Status_AddrReadNoAck 0x48	/* Adr + rd bit xmtd, NO ack rcvd */
#define	I2C_Status_MasterReadAck 0x50	/* Master rcvd data bye, ack rcvd */
#define	I2C_Status_MasterReadNoAck 0x58	/* Master rcvd data bye, NO ack rcvd */
#define	I2C_Status_2ndAddrWriteAck 0xd0	/* 2nd adr + wr bit xmid, ack rcvd */
#define	I2C_Status_2ndAddrWriteNoAck 0xd8 /* 2nd adr + wr bit xmid, NO ack rcvd */
#define	I2C_Status_2ndAddrReadAck 0xe0	/* 2nd adr + rd bit xmid, ack rcvd */
#define	I2C_Status_2ndAddrReadNoAck 0xe8 /* 2nd adr + rd bit xmtd, NO ack rcvd */
#define	I2C_Status_Idle		0xf8	/* Idle */

#endif /* _DEV_MARVELL_GTI2CREG_H_ */
