/*	$NetBSD: envctrlreg.h,v 1.2 2007/04/18 14:49:44 tnn Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tobias Nygren.
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

#define	ENVCTRL_SHELF0_ADDR	0x20	/* PCF8574, LEDs at disk shelves */
#define	ENVCTRL_SHELF1_ADDR	0x21	
#define	ENVCTRL_SHELF2_ADDR	0x22

#define	ENVCTRL_FANVOLTAGE_ADDR	0x27	/* TDA8444 */
#define	ENVCTRL_FANVOLTAGE_MAX	0x3F
#define	ENVCTRL_FANVOLTAGE_MIN	0x1F
#define	ENVCTRL_FANPORT_CPU	0
#define	ENVCTRL_FANPORT_PS	1
#define	ENVCTRL_FANPORT_AFB	2

#define	ENVCTRL_INTR_ADDR	0x38	/* PCF8574 */
#define	ENVCTRL_INTR_PS0	(1<<0)
#define	ENVCTRL_INTR_PS1	(1<<1)
#define	ENVCTRL_INTR_PS2	(1<<2)
#define	ENVCTRL_INTR_WDT_RST	(1<<3)	/* 0 = reset wdt after it has tripped */
#define	ENVCTRL_INTR_FANFAIL	(1<<4) 
#define	ENVCTRL_INTR_UNKNOWN1	(1<<5)	/* front panel? */
#define	ENVCTRL_INTR_UNKNOWN2	(1<<6)	/* pwr fail or keyswitch intr? */
#define	ENVCTRL_INTR_ENABLE	(1<<7)	/* 0 = enable interrupts to system */

#define	ENVCTRL_PS2_ADDR	0x39	/* PCF8574, power supply 2 status */
#define	ENVCTRL_PS1_ADDR	0x3A	/* PCF8574, power supply 1 status */
#define	ENVCTRL_PS0_ADDR	0x3B	/* PCF8574, power supply 0 status */
#define	ENVCTRL_PS_PRESENT	(1<<0)	/* 0 = present */
#define	ENVCTRL_PS_550W		(1<<1)	/* 0 = 550W class PS */
#define	ENVCTRL_PS_650W		(1<<2)	/* 0 = 650W class PS */
#define	ENVCTRL_PS_OK		(1<<3)	/* 1 = DC levels okay */
#define	ENVCTRL_PS_OVERLOAD	(1<<4)	/* 0 = overloaded */
#define	ENVCTRL_PS_LOADSHARE_ERROR	(1<<5)	/* 0 = load sharing error */

#define	ENVCTRL_FANFAIL_ADDR	0x3C	/* PCF8574 */
#define	ENVCTRL_FANFAIL_PS2	(1<<0)
#define	ENVCTRL_FANFAIL_PS1	(1<<1)
#define	ENVCTRL_FANFAIL_PS0	(1<<2)
#define	ENVCTRL_FANFAIL_CPU0	(1<<3)
#define	ENVCTRL_FANFAIL_CPU1	(1<<4)
#define	ENVCTRL_FANFAIL_CPU2	(1<<5)
#define	ENVCTRL_FANFAIL_AFB	(1<<6)

#define	ENVCTRL_LED_ADDR	0x3E	/* PCF8574, LEDs and keyswitch */
#define	ENVCTRL_LED_DISKERR	(1<<0)
#define	ENVCTRL_LED_PSERR	(1<<1)
#define	ENVCTRL_LED_OVERTEMP	(1<<2)
#define	ENVCTRL_LED_ERR		(1<<3)
#define	ENVCTRL_LED_ACT		(1<<4)
#define	ENVCTRL_LED_PWR		(1<<5)
#define	ENVCTRL_KEY_LOCK	(1<<6)
#define	ENVCTRL_KEY_DIAG	(1<<7)

#define	ENVCTRL_PS0TEMP_ADDR	0x48	/* PCF8591, power supply 0 temp */
#define	ENVCTRL_PS1TEMP_ADDR	0x49	/* PCF8591, power supply 1 temp */
#define	ENVCTRL_PS2TEMP_ADDR	0x4A	/* PCF8591, power supply 2 temp */
#define	ENVCTRL_AMB_ADDR	0x4D	/* LM75, ambient temperature */
#define	ENVCTRL_SOMETHING_ADDR	0x4E	/* PCF8591, not sure what it does */
#define	ENVCTRL_CPUTEMP_ADDR	0x4F	/* PCF8591, cpu temperatures */
#define	ENVCTRL_WATCHDOG_ADDR	0x50	/* PCF8583, fan regulator watchdog */
