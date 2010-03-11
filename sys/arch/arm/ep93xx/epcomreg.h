/*	$NetBSD: epcomreg.h,v 1.2.80.1 2010/03/11 15:02:05 yamt Exp $ */

/*
 * Copyright (c) 2004 Jesse Off
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
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS 
 * HEAD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _EPCOMREG_H_
#define _EPCOMREG_H_

#define EPCOM_FREQ		7372800
#define EPCOMSPEED2BRD(b)	((EPCOM_FREQ / (16 * (b))) - 1)


/* UART Data Register */
#define EPCOM_Data	0x00000000UL

/* UART Receive Status/Error Clear Register */
#define EPCOM_RXSts	0x00000004UL
#define  RXSts_FE	0x01
#define  RXSts_PE	0x02
#define  RXSts_BE	0x04
#define  RXSts_OE	0x08

/* UART Line Control Register High */
#define EPCOM_LinCtrlHigh	0x00000008UL
#define  LinCtrlHigh_BRK	0x01
#define  LinCtrlHigh_PEN	0x02
#define  LinCtrlHigh_EPS	0x04
#define  LinCtrlHigh_STP2	0x08
#define  LinCtrlHigh_FEN	0x10
#define  LinCtrlHigh_WLEN	0x60

/* UART Line Control Register Middle */
#define EPCOM_LinCtrlMid	0x0000000cUL

/* UART Line Control Register Low */
#define EPCOM_LinCtrlLow	0x00000010UL

/* UART control register */
#define EPCOM_Ctrl	0x00000014UL
#define  Ctrl_UARTE	0x01	/* UART Enable */
#define  Ctrl_MSIE	0x08	/* Modem Status Interrupt Enable */
#define  Ctrl_RIE	0x10	/* Receive Interrupt Enable */
#define  Ctrl_TIE	0x20	/* Transmit Interrupt Enable */
#define  Ctrl_RTIE	0x40	/* Receive Timeout Enable */
#define  Ctrl_LBE	0x80	/* Loopback Enable */

/* UART Flag register */
#define EPCOM_Flag	0x00000018UL
#define  Flag_CTS	0x01	/* Clear To Send status */
#define  Flag_DSR	0x02	/* Data Set Ready status */
#define  Flag_DCD	0x04	/* Data Carrier Detect status */
#define  Flag_BUSY	0x08	/* UART Busy */
#define  Flag_RXFE	0x10	/* Receive FIFO Empty */
#define  Flag_TXFF	0x20	/* Transmit FIFO Full */
#define  Flag_RXFF	0x40	/* Receive FIFO Full */
#define  Flag_TXFE	0x80	/* Transmit FIFO Empty */

/* UART Interrupt Identification and Interrupt Clear Register */
#define EPCOM_IntIDIntClr	0x0000001cUL
#define  IntIDIntClr_MIS	0x01	/* Modem Interrupt Status */
#define  IntIDIntClr_RIS	0x01	/* Receive Interrupt Status */
#define  IntIDIntClr_TIS	0x01	/* Transmit Interrupt Status */
#define  IntIDIntClr_RTIS	0x01	/* Receive Timeout Interrupt Status */

/* UART Modem Control Register */
#define EPCOM_ModemCtrl	0x00000100UL
#define  ModemCtrl_DTR	0x01	/* DTR output signal */
#define  ModemCtrl_RTS	0x02	/* RTS output signal */

/* UART Modem Status Register */
#define EPCOM_ModemSts	0x00000104UL
#define  ModemSts_DCTS	0x01	/* Delta CTS */
#define  ModemSts_DDSR	0x02	/* Delta DSR */
#define  ModemSts_TERI	0x04	/* Trailing Edge Ring Indicator */
#define  ModemSts_DDCD	0x08	/* Delta DCD */
#define  ModemSts_CTS	0x10	/* Inverse CTSn input pin */
#define  ModemSts_DSR	0x20	/* Inverse of the DSRn pin */
#define  ModemSts_RI	0x40	/* Inverse of RI input pin */
#define  ModemSts_DCD	0x80	/* Inverse of DCDn input pin */

#endif /* _EPCOMREG_H_ */
