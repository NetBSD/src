/* $NetBSD: wii.h,v 1.7.2.2 2024/02/03 11:47:08 martin Exp $ */

/*-
 * Copyright (c) 2024 Jared McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Nintendo Wii platform definitions.
 */

#ifndef _WII_H
#define _WII_H

#include <powerpc/pio.h>

#define WII_MEM1_BASE			0x00000000
#define WII_MEM1_SIZE			0x01800000	/* 24 MB */
#define WII_MEM2_BASE			0x10000000
#define WII_MEM2_SIZE			0x04000000	/* 64 MB */

#define WII_IOMEM_BASE			0x0c000000

#define GLOBAL_BASE			0x00000000
#define GLOBAL_SIZE			0x00003400

#define BROADWAY_BASE			0x0c000000
#define BROADWAY_SIZE			0x00000004

#define VI_BASE				0x0c002000
#define VI_SIZE				0x00000100

#define PI_BASE				0x0c003000
#define PI_SIZE				0x00000100

#define DSP_BASE			0x0c005000
#define DSP_SIZE			0x00000200

#define EXI_BASE			0x0d006800
#define EXI_SIZE			0x00000080

#define AI_BASE				0x0d006c00
#define AI_SIZE				0x00000020

#define HOLLYWOOD_BASE			0x0d000000
#define HOLLYWOOD_PRIV_BASE		0x0d800000
#define HOLLYWOOD_SIZE			0x00008000

#define XFB_START			0x01698000
#define XFB_SIZE			0x00168000

#define DSP_MEM_START			0x10000000
#define DSP_MEM_SIZE			0x00004000

#define IPC_START			0x133e0000
#define IPC_SIZE			0x00020000

#define ARM_START			0x13400000
#define ARM_SIZE			0x00c00000

#define BUS_FREQ_HZ			243000000
#define CPU_FREQ_HZ			(BUS_FREQ_HZ * 3)
#define TIMEBASE_FREQ_HZ		(BUS_FREQ_HZ / 4)

/* Global memory structure */
#define GLOBAL_MEM1_SIZE		(GLOBAL_BASE + 0x0028)
#define GLOBAL_CUR_VID_MODE		(GLOBAL_BASE + 0x00cc)
#define GLOBAL_BUS_SPEED		(GLOBAL_BASE + 0x00f8)
#define GLOBAL_CPU_SPEED		(GLOBAL_BASE + 0x00fc)
#define GLOBAL_SYSTEM_TIME		(GLOBAL_BASE + 0x30d8)
#define GLOBAL_MEM2_SIZE		(GLOBAL_BASE + 0x3118)
#define GLOBAL_MEM2_AVAIL_START		(GLOBAL_BASE + 0x3124)
#define GLOBAL_MEM2_AVAIL_END		(GLOBAL_BASE + 0x3128)
#define GLOBAL_IOS_VERSION		(GLOBAL_BASE + 0x3140)

/* Processor interface registers */
#define PI_INTSR			(PI_BASE + 0x00)
#define PI_INTMR			(PI_BASE + 0x04)

/* Processor IRQs */
#define PI_IRQ_EXI			4
#define PI_IRQ_AI			5
#define PI_IRQ_DSP			6
#define PI_IRQ_HOLLYWOOD		14

/* Hollywood registers */
#define HW_VIDIM			(HOLLYWOOD_PRIV_BASE + 0x01c)
#define  VIDIM_E			__BIT(7)
#define  VIDIM_Y			__BITS(5,3)
#define  VIDIM_C			__BITS(2,0)
#define HW_PPCIRQFLAGS			(HOLLYWOOD_BASE + 0x030)
#define HW_PPCIRQMASK			(HOLLYWOOD_BASE + 0x034)
#define HW_ARMIRQFLAGS			(HOLLYWOOD_PRIV_BASE + 0x038)
#define HW_ARMIRQMASK			(HOLLYWOOD_PRIV_BASE + 0x03c)
#define HW_AHBPROT			(HOLLYWOOD_PRIV_BASE + 0x064)
#define HW_GPIOB_OUT			(HOLLYWOOD_BASE + 0x0c0)
#define HW_GPIOB_DIR			(HOLLYWOOD_BASE + 0x0c4)
#define HW_GPIOB_IN			(HOLLYWOOD_BASE + 0x0c8)
#define HW_GPIO_OWNER			(HOLLYWOOD_PRIV_BASE + 0x0fc)
#define HW_RESETS			(HOLLYWOOD_PRIV_BASE + 0x194)
#define  RSTB_IOP			__BIT(23)
#define  RSTBINB			__BIT(0)
#define HW_VERSION			(HOLLYWOOD_BASE + 0x214)
#define  HWVER_MASK			__BITS(7,4)
#define  HWREV_MASK			__BITS(3,0)

/* GPIOs */
#define GPIO_SHUTDOWN			1
#define GPIO_SLOT_LED			5

/* Command line protocol */
#define WII_ARGV_MAGIC			0x5f617267
struct wii_argv {
	uint32_t	magic;
	uint32_t	cmdline;
	uint32_t	length;
	uint32_t	unused[3];
};

/* Blink the slot LED forever at the specified interval. */
static inline void __dead
wii_slot_led_blink(u_int interval_us)
{
	uint32_t val;

	for (val = in32(HW_GPIOB_OUT); ; val ^= __BIT(GPIO_SLOT_LED)) {
		delay(interval_us);
		out32(HW_GPIOB_OUT, val);
	}
}

#endif /* !_WII_H */
