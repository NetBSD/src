/*	$NetBSD: ralink_gpio.h,v 1.2.12.1 2017/12/03 11:36:28 jdolecek Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RALINK_GPIO_H_
#define _RALINK_GPIO_H_

/* ra_gpio.h -- Ralink 3052 gpio driver public defines */

/* Board-specific details */

#if defined(TABLEROCK) || defined(SPOT2) || defined(PUCK) || defined(MOAB)

/*
 * Enable pins:		UART Full, and GPIO0 pins 0-23
 * Rising edge:		three buttons & dock sense
 * Falling edge:	three buttons & dock sense
 */
#define GPIO_TR_PIN_MASK            0x017f81
#define GPIO_TR_OUTPUT_PIN_MASK     0x003b80
#define GPIO_TR_INT_PIN_MASK        0x014401
#define GPIO_TR_INT_FEDGE_PIN_MASK  0x014401
#define GPIO_TR_POL_MASK            0x000000

/*
 * Enable pins:		RGMII, and SDRAM GPIO pins 24-51
 * Rising edge:		IN_5V, DOCK_SENSE and LAN_WAN_SWITCH
 * Falling edge:	IN_5V, DOCK_SENSE and LAN_WAN_SWITCH
 */
#define GPIO_TR_PIN_MASK_24_51            0x0affff3f
#define GPIO_TR_OUTPUT_PIN_MASK_24_51     0x00ffff07
#define GPIO_TR_INT_PIN_MASK_24_51        0x02000010
#define GPIO_TR_INT_FEDGE_PIN_MASK_24_51  0x02000010
#define GPIO_TR_POL_MASK_24_51            0x00000000

#define WPS_BUTTON          0		/* input, interrupt */

/* UARTF Block */
#define POWER_EN_3G         7
#define POWER_EN_WIMAX      8
#define POWER_ON            9
#define SOFT_RST_IN_BUTTON 10		/* input, interrupt */
#define NC_4               11
#define USB_SW1            12
#define USB_SW2            13
#define POWER_OFF_BUTTON   14		/* input, interrupt */

/* UARTL Block */
#define NC_6               15
#define DOCK_SENSE         16		/* input, interrupt */

/* SDRAM block */
#define DOCK_5V_ENA        24
#define USB5V_EN           25
#define CHARGER_OFF        26		/* 1 : charger on, 0 : charger off */
#define PA_OR_PC_IN        27		/* input */
#define IN_5V              28		/* input, interrupt */
#define CHRGB              29		/* input, NI for P1 board */
#define NC_1               30
#define NC_2               31
#define LED_BATT_7         32
#define LED_BATT_8         33
#define LED_BATT_9         34
#define LED_BATT_10        35
#define LED_SS_11          36
#define LED_SS_12          37
#define LED_SS_13          38
#define LED_SS_14          39

/* RGMII block */
#define LED_WPS            40
#define LED_WIFI           41
#define LED_CHARGE         42
#define LED_POWER          43
#define LED_WIMAX          44
#define LED_3G             45
#define POWER_ON_LATCH     46
#define HUB_RST            47
#define NC_3               48
#define LAN_WAN_SW         49		/* input, interrupt */
#define NC_5               50
#define PUCK_BATTERY_LOW   51

/*
 * Debounced pins:
 *	WPS_BUTTON, POWER_OFF_BUTTON, DOCK_SENSE, IN_5V, LAN_WAN_SW
 */
#define DEBOUNCED_PINS 5

#define GPIO_PIN_MASK GPIO_TR_PIN_MASK
#define GPIO_OUTPUT_PIN_MASK GPIO_TR_OUTPUT_PIN_MASK
#define GPIO_INT_PIN_MASK GPIO_TR_INT_PIN_MASK
#define GPIO_INT_FEDGE_PIN_MASK GPIO_TR_INT_FEDGE_PIN_MASK
#define GPIO_POL_MASK GPIO_TR_POL_MASK

#define GPIO_PIN_MASK_24_51 GPIO_TR_PIN_MASK_24_51
#define GPIO_OUTPUT_PIN_MASK_24_51 GPIO_TR_OUTPUT_PIN_MASK_24_51
#define GPIO_INT_PIN_MASK_24_51 GPIO_TR_INT_PIN_MASK_24_51
#define GPIO_INT_FEDGE_PIN_MASK_24_51 GPIO_TR_INT_FEDGE_PIN_MASK_24_51
#define GPIO_POL_MASK_24_51 GPIO_TR_POL_MASK_24_51

#define GPIO_PIN_MASK_72_95           0
#define GPIO_OUTPUT_PIN_MASK_72_95    0
#define GPIO_INT_PIN_MASK_72_95       0
#define GPIO_INT_FEDGE_PIN_MASK_72_95 0
#define GPIO_POL_MASK_72_95           0

#endif	/* !TABLEROCK */

#if defined(PEBBLES500) || defined(PEBBLES35)

/*
 * Enable pins:		I2C, UART Full, and GPIO0 pins 0-23
 * Rising edge:		?
 * Falling edge:	buttons
 */
#define GPIO_PB500_PIN_MASK            0x005d83
#define GPIO_PB500_OUTPUT_PIN_MASK     0x000083
#define GPIO_PB500_INT_PIN_MASK        0x005c00
#define GPIO_PB500_INT_FEDGE_PIN_MASK  0x000500
#define GPIO_PB500_POL_MASK            0x000000

/* Enable RGMII */
#define P3_HARDWARE
#if defined(P3_HARDWARE)
#define GPIO_PB500_PIN_MASK_24_51            0x03cafe00
#define GPIO_PB500_OUTPUT_PIN_MASK_24_51     0x03c8fe00
#else
#define GPIO_PB500_PIN_MASK_24_51            0x0fff0000
#define GPIO_PB500_OUTPUT_PIN_MASK_24_51     0x0ffd0000
#endif
#define GPIO_PB500_INT_PIN_MASK_24_51        0x00020000   /* rising edge ints */
#define GPIO_PB500_INT_FEDGE_PIN_MASK_24_51  0x00020000
#define GPIO_PB500_POL_MASK_24_51            0x00000000

/* I2C block */
#define POWER_EN_EXCARD1_3_3v  1
#define POWER_EN_EXCARD1_1_5v  2

/* UARTF Block */
#define VBUS_EN                    7
#define WPS_BUTTON                 8         /* input, interrupt */
#define SOFT_RST_IN_BUTTON        10         /* input, interrupt */
#define CURRENT_LIMIT_FLAG1_3_3v  11         /* input, interrupt */
#define CURRENT_LIMIT_FLAG_USB1   12         /* input, interrupt */
#define CURRENT_LIMIT_FLAG1_1_5v  14         /* input, interrupt */

/* SDRAM block */
#if defined(P3_HARDWARE)
#define LAN_WAN            33
#define LED_WIFI           34
#define LED_WPS            35
#define LED_USB            36
#define LED_USB_RED        37
#if defined(PEBBLES500)
#define LED_EXP            38
#define LED_EXP_RED        39
#endif
#else	/* P3_HARDWARE */
#define LED_WPS            45
#define LED_WIFI           42
#define LED_USB            40
#define LED_USB_RED        44
#if defined(PEBBLES500)
#define LED_EXP            50
#define LED_EXP_RED        51
#endif
#endif	/* P3_HARDWARE */

/* RGMII block */
#define EXCARD_ATTACH      41         /* input, interrupt */
#define POWER_EN_USB       43
#if defined(PEBBLES500)
#define LED_SS_13          46
#define LED_SS_12          47
#define LED_SS_11          48
#define LED_SS_10          49
#endif

/*
 * Debounced Pins:
 *	WPS_BUTTON, SOFT_RST_IN_BUTTON, CURRENT_LIMIT_FLAG_USB,
 *	CURRENT_LIMIT_FLAG3_3, CURRENT_LIMIT_FLAG1_5, EXCARD_ATTACH
 */
#define DEBOUNCED_PINS 6

#define GPIO_PIN_MASK GPIO_PB500_PIN_MASK
#define GPIO_OUTPUT_PIN_MASK GPIO_PB500_OUTPUT_PIN_MASK
#define GPIO_INT_PIN_MASK GPIO_PB500_INT_PIN_MASK
#define GPIO_INT_FEDGE_PIN_MASK GPIO_PB500_INT_FEDGE_PIN_MASK
#define GPIO_POL_MASK GPIO_PB500_POL_MASK

#define GPIO_PIN_MASK_24_51 GPIO_PB500_PIN_MASK_24_51
#define GPIO_OUTPUT_PIN_MASK_24_51 GPIO_PB500_OUTPUT_PIN_MASK_24_51
#define GPIO_INT_PIN_MASK_24_51 GPIO_PB500_INT_PIN_MASK_24_51
#define GPIO_INT_FEDGE_PIN_MASK_24_51 GPIO_PB500_INT_FEDGE_PIN_MASK_24_51
#define GPIO_POL_MASK_24_51 GPIO_PB500_POL_MASK_24_51

#define GPIO_PIN_MASK_72_95           0
#define GPIO_OUTPUT_PIN_MASK_72_95    0
#define GPIO_INT_PIN_MASK_72_95       0
#define GPIO_INT_FEDGE_PIN_MASK_72_95 0
#define GPIO_POL_MASK_72_95           0

#endif	/*  TABLEROCK || SPOT2 || PUCK || MOAB */

#if defined(SLICKROCK)

/*
 * Enable:	I2C, UART Full, and GPIO0 pins 0-23
 * Rising edge:		buttons, overcurrent, switch
 * Falling edge:	buttons, overcurrent, switch
 */
#define GPIO_SR_PIN_MASK            0x007fcf
#define GPIO_SR_OUTPUT_PIN_MASK     0x00128d
#define GPIO_SR_INT_PIN_MASK        0x006d42
#define GPIO_SR_INT_FEDGE_PIN_MASK  0x006c42
#define GPIO_SR_POL_MASK            0x000002

/* Enable RGMII */
#define GPIO_SR_PIN_MASK_24_51        0x0000387f
#define GPIO_SR_OUTPUT_PIN_MASK_24_51 0x0000387b
#define GPIO_SR_INT_PIN_MASK_24_51        0x0004
#define GPIO_SR_INT_FEDGE_PIN_MASK_24_51  0x0004
#define GPIO_SR_POL_MASK_24_51            0x00000000

#define GPIO_SR_PIN_MASK_72_95        0x00000fff
#define GPIO_SR_OUTPUT_PIN_MASK_72_95 0x00000ff7
#define GPIO_SR_INT_PIN_MASK_72_95        0x0008
#define GPIO_SR_INT_FEDGE_PIN_MASK_72_95  0x0008
#define GPIO_SR_POL_MASK_72_95            0x00000000

#define LED_USB2_G          0

/* I2C block */
#define WIFI_ENABLE                1
#define EX2_CPUSB_RST              2

/* SPI block */
#define POWER_EN_USB3              3
#define CURRENT_LIMIT_FLAG_USB3    6         /* input, interrupt */

/* UARTF Block */
#define EX1_CPUSB_RST              7
#define SS_BUTTON                  8         /* input, interrupt */
#define POWER_EN_USB1              9
#define SOFT_RST_IN_BUTTON        10         /* input, interrupt */
#define CURRENT_LIMIT_FLAG_USB2   11         /* input, interrupt */
#define POWER_EN_USB2             12
#define CURRENT_LIMIT_FLAG_EX2    13         /* input, interrupt */
					     /*  (pin 76 on P1 boards) */
#define CURRENT_LIMIT_FLAG_USB1   14         /* input, interrupt */

/* GPIO */
#define LED_USB1_G         24
#define LED_USB1_R         25
#define WPS_BUTTON         26
#define LED_EX1_R          27
#define LED_EX2_G          28
#define LED_EX2_R          29
#define LED_WIFI_RED       30

/* LNA_PE_Gx */
#define LED_USB3_G         35
#define LED_USB3_R         36
#define LED_EX1_G          37

/* RGMII2 */
#define POWER_EN_EX1       74
#define CURRENT_LIMIT_FLAG_EX1 75  /* input, interrupt */
#define LED_USB2_R         76
#define POWER_EN_EX2       77
#define LED_POWER          78
#define LED_WPS            79
#define LED_WIFI_BLUE      82
#define LED_WIFI           83

#define LED_SS_10          73
#define LED_SS_11          80
#define LED_SS_12          81
#define LED_SS_13          72

/* Debounced Pins:
 *	WPS_BUTTON, SOFT_RST_IN_BUTTON, SS_BUTTON
 *	CURRENT_LIMIT_FLAG USB * 3 + EXP * 2
 */
#define DEBOUNCED_PINS 9

#define GPIO_PIN_MASK GPIO_SR_PIN_MASK
#define GPIO_OUTPUT_PIN_MASK GPIO_SR_OUTPUT_PIN_MASK
#define GPIO_INT_PIN_MASK GPIO_SR_INT_PIN_MASK
#define GPIO_INT_FEDGE_PIN_MASK GPIO_SR_INT_FEDGE_PIN_MASK
#define GPIO_POL_MASK GPIO_SR_POL_MASK

#define GPIO_PIN_MASK_24_51 GPIO_SR_PIN_MASK_24_51
#define GPIO_OUTPUT_PIN_MASK_24_51 GPIO_SR_OUTPUT_PIN_MASK_24_51
#define GPIO_INT_PIN_MASK_24_51 GPIO_SR_INT_PIN_MASK_24_51
#define GPIO_INT_FEDGE_PIN_MASK_24_51 GPIO_SR_INT_FEDGE_PIN_MASK_24_51
#define GPIO_POL_MASK_24_51 GPIO_SR_POL_MASK_24_51

#define GPIO_PIN_MASK_72_95 GPIO_SR_PIN_MASK_72_95
#define GPIO_OUTPUT_PIN_MASK_72_95 GPIO_SR_OUTPUT_PIN_MASK_72_95
#define GPIO_INT_PIN_MASK_72_95 GPIO_SR_INT_PIN_MASK_72_95
#define GPIO_INT_FEDGE_PIN_MASK_72_95 GPIO_SR_INT_FEDGE_PIN_MASK_72_95
#define GPIO_POL_MASK_72_95 GPIO_SR_POL_MASK_72_95

#endif	/* SLICKROCK */


/* Exported functions */
extern int ra_gpio_pin_read(void *arg, int pin);
extern void ra_gpio_pin_write(void *arg, int pin, int value);

/* Kernel Events (platform-neutral) */
#define WPS_BUTTON_EVT 1
#define RESET_BUTTON_EVT 2
#define POWER_BUTTON_EVT 3
#define IN_5V_EVT 4
#if 0
#define PWR_FLAG_3G_EVT 5
#endif
#define DOCK_SENSE_EVT 6
#define LAN_WAN_SW_EVT 7
#define WIFI_ENABLE_EVT 8
#define SS_BUTTON_EVT 9
#define CURRENT_LIMIT_EVT 10
#define EXCARD_ATTACH_EVT 11

#endif	/* _RALINK_GPIO_H_ */
