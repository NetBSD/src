/*	$NetBSD: zynq7000_reg.h,v 1.1.20.2 2017/12/03 11:35:57 jdolecek Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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

#ifndef _ARM_ZYNQ_ZYNQ7000_REG_H_
#define _ARM_ZYNQ_ZYNQ7000_REG_H_

#define	ZYNQ7000_IOREG_PBASE	0xe0000000
#define	ZYNQ7000_IOREG_SIZE	0x00200000

#define	ZYNQ7000_ARMCORE_PBASE	0xf8f00000
#define	ZYNQ7000_ARMCORE_SIZE	0x00100000

#define	ZYNQ7000_IO_SIZE	(ZYNQ7000_IOREG_SIZE + ZYNQ7000_ARMCORE_SIZE)

#define SLCR_BASE		0xf8000000
#define SLCR_SIZE		0x00000b78

#define UART0_BASE		0xe0000000
#define UART1_BASE		0xe0001000
#define UART_SIZE		0x00000048

#define	ARMCORE_SCU_BASE	0x00000000
#define ARMCORE_L2C_BASE	0x00002000

#define USB0_BASE		0xe0002000
#define USB1_BASE		0xe0003000
#define USB_SIZE		0x00000200

#define SDIO0_BASE		0xe0100000
#define SDIO1_BASE		0xe0101000
#define SDIO_SIZE		0x00001000

#endif /* _ARM_ZYNQ_ZYNQ7000_REG_H_ */
