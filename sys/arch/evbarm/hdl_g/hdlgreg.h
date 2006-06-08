/*	$NetBSD: hdlgreg.h,v 1.2 2006/06/08 23:27:47 nonaka Exp $	*/

/*
 * Copyright (c) 2005, 2006 Kimihiro Nonaka
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _HDLGREG_H_
#define	_HDLGREG_H_

/*
 * Memory map and register definitions for I-O DATA HDL-G
 */

/*
 * The memory map of I/O-DATA HDL-G looks like so:
 *
 *           ------------------------------
 *		Intel 80321 IOP Reserved
 * FFFF E900 ------------------------------
 *		Peripheral Memory Mapped
 *		    Registers
 * FFFF E000 ------------------------------
 *		On-board devices
 * FE80 0000 ------------------------------
 *		SDRAM
 * A000 0000 ------------------------------
 *		Reserved
 * 9100 0000 ------------------------------
 * 		Flash
 * 9080 0000 ------------------------------
 *		Reserved
 * 9002 0000 ------------------------------
 *		ATU Outbound Transaction
 *		    Windows
 * 8000 0000 ------------------------------
 *		ATU Outbound Direct
 *		    Addressing Windows
 * 0000 1000 ------------------------------
 *		Initialization Boot Code
 *		    from Flash
 * 0000 0000 ------------------------------
 */

/*
 * We allocate a page table for VA 0xfe400000 (4MB) and map the
 * PCI I/O space (64K) and i80321 memory-mapped registers (4K) there.
 */
#define	HDLG_IOPXS_VBASE	0xfe400000UL
#define	HDLG_IOW_VBASE		HDLG_IOPXS_VBASE
#define	HDLG_80321_VBASE	(HDLG_IOW_VBASE + \
				 VERDE_OUT_XLATE_IO_WIN_SIZE)

/*
 * The GIGALANDISK on-board devices are mapped VA==PA during bootstrap.
 * Conveniently, the size of the on-board register space is 1 section
 * mapping.
 */
#define	HDLG_OBIO_BASE	0xfe800000UL
#define	HDLG_OBIO_SIZE	0x00100000UL	/* 1MB */

#define	HDLG_UART1	0xfe800000UL	/* TI 16550 */
#define	HDLG_PLD	0xfe8d0000UL	/* CPLD */

/*
 * CPLD
 */
#define	HDLG_LEDCTRL		(HDLG_PLD + 0x00)
#define		LEDCTRL_STAT_GREEN	0x01
#define		LEDCTRL_STAT_RED	0x02
#define		LEDCTRL_USB1		0x04
#define		LEDCTRL_USB2		0x08
#define		LEDCTRL_USB3		0x10
#define		LEDCTRL_USB4		0x20
#define		LEDCTRL_HDD		0x40
#define		LEDCTRL_BUZZER		0x80
#define	HDLG_PWRLEDCTRL		(HDLG_PLD + 0x01)
#define		PWRLEDCTRL_0		0x01
#define		PWRLEDCTRL_1		0x02
#define		PWRLEDCTRL_2		0x04
#define		PWRLEDCTRL_3		0x08
#define	HDLG_BTNSTAT		(HDLG_PLD + 0x02)
#define		BTNSTAT_POWER		0x01
#define		BTNSTAT_SELECT		0x02
#define		BTNSTAT_COPY		0x04
#define		BTNSTAT_REMOVE		0x08
#define		BTNSTAT_RESET		0x10
#define	HDLG_INTEN		(HDLG_PLD + 0x03)
#define		INTEN_PWRSW		0x01
#define		INTEN_BUTTON		0x02
#define		INTEN_RTC		0x40
#define	HDLG_PWRMNG		(HDLG_PLD + 0x04)
#define		PWRMNG_POWOFF		0x01
#define		PWRMNG_RESET		0x02
#define	HDLG_FANCTRL		(HDLG_PLD + 0x06)
#define		FANCTRL_OFF		0x00
#define		FANCTRL_ON		0x01

#define	hdlg_enable_pldintr(bit) \
do { \
	*(volatile uint8_t *)HDLG_INTEN |= (bit); \
} while (/*CONSTCOND*/0)

#define	hdlg_disable_pldintr(bit) \
do { \
	*(volatile uint8_t *)HDLG_INTEN &= ~(bit); \
} while (/*CONSTCOND*/0)

#endif /* _HDLGREG_H_ */
