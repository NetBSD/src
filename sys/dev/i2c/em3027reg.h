/*	$NetBSD: em3027reg.h,v 1.1 2018/01/05 03:07:15 uwe Exp $ */
/*
 * Copyright (c) 2018 Valery Ushakov
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

/*
 * EM Microelectronic EM3027 RTC
 */
#ifndef _EM3027REG_H_
#define _EM3027REG_H_

#define EM3027_ADDR	0x56


/*
 * Control Page: 0
 */
#define EM3027_ONOFF		0x00
#define   EM3027_ONOFF_CLKOUT		0x80	/* 0: IRQ, 1: CLK */
#define   EM3027_ONOFF_SR		0x10	/* self-recovery */
#define   EM3027_ONOFF_EEREF		0x08	/* EEPROM self-refresh */
#define   EM3027_ONOFF_TR		0x04   	/* timer auto-reload */
#define   EM3027_ONOFF_TI		0x02	/* timer */
#define   EM3027_ONOFF_WA		0x01	/* watch */

#define EM3027_IRQ_CTL		0x01
#define EM3027_IRQ_FLAGS	0x02

/* For both EM3027_IRQ_CTL and EM3027_IRQ_FLAGS */
#define   EM3027_IRQ_SR			0x10	/* self-recovery */
#define   EM3027_IRQ_V2			0x08	/* VLow2 */
#define   EM3027_IRQ_V1			0x04	/* VLow1 */
#define   EM3027_IRQ_TINT		0x02	/* timer */
#define   EM3027_IRQ_AINT		0x01	/* alarm */

#define EM3027_STATUS		0x03
#define   EM3027_STATUS_EEBUSY		0x80	/* r/o: write or self-refresh */
#define   EM3027_STATUS_POWER_ON	0x20
#define   EM3027_STATUS_RESET		0x10	/* after reset or recovery */
#define   EM3027_STATUS_VLOW2		0x08	/* voltage lost */
#define   EM3027_STATUS_VLOW1		0x04	/* voltage low */

/* Request system reset */
#define EM3027_RESET		0x04
#define   EM3027_RESET_SYSRES		0x10


/*
 * Watch Page: 1
 */
#define EM3027_WATCH_SEC	0x08	/* 0..59 */
#define EM3027_WATCH_MIN	0x09	/* 0..59 */
#define EM3027_WATCH_HOUR	0x0a	/* 0..23 or 1..12 */
#define   EM3027_WATCH_HOUR_S12		0x40	/* select 12/24 hours */
#define   EM3027_WATCH_HOUR_PM		0x20	/* am/pm if 12 hours */
#define EM3027_WATCH_DAY	0x0b	/* 1..31 */
#define EM3027_WATCH_WDAY	0x0c	/* 1..7 */
#define EM3027_WATCH_MON	0x0d	/* 1..12 */
#define EM3027_WATCH_YEAR	0x0e	/* 0..79 */


/*
 * Alarm Page: 2
 *
 * Same format as watch registers except there's no S12 bit in the
 * hours register and the upper bit (EM3027_ALARM_ENABLE) in each
 * register selects it for comparison.
 */
#define EM3027_ALARM_SEC	0x10
#define EM3027_ALARM_MIN	0x11
#define EM3027_ALARM_HOUR	0x12
#define EM3027_ALARM_DATE	0x13
#define EM3027_ALARM_DAYS	0x14
#define EM3027_ALARM_MON	0x15
#define EM3027_ALARM_YEAR	0x16

/* MSB in each alarm register enables comparison */
#define   EM3027_ALARM_ENABLE		0x80


/*
 * Timer Page: 3
 */
#define EM3027_TIMER_LO		0x18
#define EM3027_TIMER_HI		0x19


/*
 * Temperature Page: 4
 */
#define EM3027_TEMP		0x20	/* -60..195C */
#define   EM3027_TEMP_BASE		(-60)


/*
 * EEPROM Control Page: 6
 */
#define EM3027_EEPROM_CTL	0x30

/* Trickle charger resistors */
#define   EM3027_EEPROM_CHARGER_MASK	0xf0
#define   EM3027_EEPROM_R80K		0x80
#define   EM3027_EEPROM_R20K		0x40
#define   EM3027_EEPROM_R5K		0x20
#define   EM3027_EEPROM_R1_5K		0x10

/* Frequency compensated for temperature (0x00 selects raw
   uncompensated 32768KHz */
#define   EM3027_EEPROM_FREQ_MASK	0x0c
#define   EM3027_EEPROM_FREQ_1HZ	0x0c
#define   EM3027_EEPROM_FREQ_32HZ	0x08
#define   EM3027_EEPROM_FREQ_1024HZ	0x04

#define   EM3027_EEPROM_THERM_ENABLE	0x02
#define   EM3027_EEPROM_THERM_PERIOD	0x01	/* 0: 1s, 1: 16s */

#endif	/* _EM3027REG_H_ */
