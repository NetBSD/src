/*	$NetBSD: wzero3_reg.h,v 1.6.14.1 2012/02/18 07:32:09 mrg Exp $	*/

/*-
 * Copyright (C) 2008, 2009, 2010 NONAKA Kimihiro <nonaka@netbsd.org>
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

#ifndef	_HPCARM_DEV_WZERO3_REG_H_
#define	_HPCARM_DEV_WZERO3_REG_H_

/* GPIO --------------------------------------------------------------------- */

/*
 * WS003SH/WS004SH specific parameter
 */
#define	GPIO_WS003SH_SD_DETECT		9	/* In */
#define	GPIO_WS003SH_TOUCH_PANEL	11	/* In */
#define	GPIO_WS003SH_SLIDE		12	/* In */
#define	GPIO_WS003SH_FULLKEY_LED	17	/* Out: H:ON, L:OFF */
#define	GPIO_WS003SH_ANTENNA_LED	37	/* Out: H:ON, L:OFF */
#define	GPIO_WS003SH_MAX1233_CS		48	/* Out */
#define	GPIO_WS003SH_RESET		89	/* Out */
#define	GPIO_WS003SH_POWER_BUTTON	95	/* In */
#define	GPIO_WS003SH_VIB		97	/* Out */
#define	GPIO_WS003SH_USB_CLIENT_DETECT	103	/* In */
#define	GPIO_WS003SH_SD_POWER		107	/* Out: H:ON, L:OFF */
#define	GPIO_WS003SH_CHARGE		114	/* Out: H:ON, L:OFF */
#define	GPIO_WS003SH_AC_DETECT		115	/* In */

/* WS003SH: GPIO OUT pin
 * CLR: 10,16,18,19,20,21,23,33,40,56,87,90,91,104,119,120
 * SET: 39,54,57
 */

/*
 * WS007SH specific parameter
 */
#define	GPIO_WS007SH_RESET_BUTTON	1	/* In: L: press, H: release */
#define	GPIO_WS007SH_POWER_BUTTON	9	/* In */
#define	GPIO_WS007SH_TOUCH_PANEL	21	/* In */
#define	GPIO_WS007SH_ADS7846_CS		33	/* Out */
#define	GPIO_WS007SH_USB_CLIENT_DETECT	35	/* In */
#define	GPIO_WS007SH_USB_HOST_POWER	37	/* Out */
#define	GPIO_WS007SH_USB_HOST_DETECT	41	/* In */
#define	GPIO_WS007SH_SD_DETECT		48	/* In */
#define	GPIO_WS007SH_HSYNC		75	/* In */
#define	GPIO_WS007SH_SLIDE		104	/* In */
#define	GPIO_WS007SH_SD_POWER		107	/* Out: H:ON, L:OFF */

/*
 * WS011SH specific parameter
 */
/*
port	I/O(Active)	name 	desc
1	I(?)		RESET_BTN button detect: reset (on: release, off:press)
9	I(?)		PWR_BTN button detect: power-on (on: press, off:release)
21	I(?)		TPANEL touch panel (on: release, off: press)
37	O		USBH_PWR USB Host power (H: enable, L: disable)
41	I(L)		USBH_DET USB Host cable detect (on: remove, off: insert)
48	I(L)		SD_DET microSD card detect (on: remove, off: insert)
51	I(?)		SLIDE LCD slider (on: open, off: close)
52	I(?)		KEYLOCK key lock slider (on: unlock, off:lock)
57	I(?)		EXCRWAL_DET
81	I(L)		EPDET earphone adapter detect (on: remove, off: insert)
91	I(?)		FULLKEYBOARD?
96	I(?)		JACKET_DET jacket detect (on: close, off: open)
?105	I(?)		WSIM_DET W-SIM detect (on: insert, off: remove)
?106	I(?)		WSIM? (same as GPIO#105?)
107	O(?)		SD_PWR: SD Card power (on: on, off: off)
115	I(H)		ACDET AC adapter detect (on: insert, off: remove)
116	I(?)		USBC_DET USB Client cable detect (on: insert, off: remove)
 */
#define	GPIO_WS011SH_RESET_BUTTON	1	/* In */
#define	GPIO_WS011SH_POWER_BUTTON	9	/* In */
#define	GPIO_WS011SH_KEYPAD		14	/* In */
#define	GPIO_WS011SH_TOUCH_PANEL	21	/* In */
#define	GPIO_WS011SH_AK4184_CS		33	/* Out */
#define	GPIO_WS011SH_USB_HOST_POWER	37	/* Out */
#define	GPIO_WS011SH_USB_HOST_DETECT	41	/* In */
#define	GPIO_WS011SH_SD_DETECT		48	/* In */
#define	GPIO_WS011SH_SLIDE		51	/* In */
#define	GPIO_WS011SH_KEY_LOCK		52	/* In */
#define	GPIO_WS011SH_HSYNC		75	/* In */
#define	GPIO_WS011SH_SD_POWER		107	/* Out */
#define	GPIO_WS011SH_USB_CLIENT_DETECT	116	/* In */

/*
 * WS020SH specific parameter
 */
#define	GPIO_WS020SH_RESET_BUTTON	1	/* In */
#define	GPIO_WS020SH_TOUCH_PANEL	21	/* In */
#define	GPIO_WS020SH_USB_HOST_DETECT	41	/* In */
#define	GPIO_WS020SH_SD_DETECT		48	/* In */
#define	GPIO_WS020SH_SLIDE		51	/* In */
#define	GPIO_WS020SH_KEY_LOCK		52	/* In */
#define	GPIO_WS020SH_POWER_BUTTON	55	/* In */
#define	GPIO_WS020SH_SD_POWER		107	/* Out */
#define	GPIO_WS020SH_USB_CLIENT_DETECT	116	/* In */

#endif	/* _HPCARM_DEV_WZERO3_REG_H_ */
