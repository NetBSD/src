/*      $NetBSD: clockreg.h,v 1.2 2003/01/18 06:09:55 thorpej Exp $        */
/*
 * Copyright (c) 1997 Rolf Grossmann
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * NeXT clock registers
 */

struct scr2 {                           /* zeroed at power-on, read/write */
    u_int s_dsp_reset : 1,
	s_dsp_block_end : 1,
	s_dsp_unpacked : 1,
	s_dsp_mode_B : 1,
	s_dsp_mode_A : 1,
	s_remote_int : 1,
	s_local_int : 2,	/* in fact :1 reserved and :1 local_int? */
	s_dram_256K : 4,
	s_dram_1M : 4,
	s_timer_on_ipl7 : 1,
	s_rom_wait_states : 3,
	s_rom_1M : 1,
	s_rtdata : 1,
	s_rtclk : 1,
	s_rtce : 1,
	s_rom_overlay : 1,
	s_dsp_int_en : 1,
	s_dsp_mem_en : 1,
	s_reserved : 4,
	s_led : 1;
};

#define	SCR2_DSP_RESET		0x80000000
#define SCR2_DSP_BLOCK_END	0x40000000
#define SCR2_DSP_UNPACKED	0x20000000
#define	SCR2_DSP_MODE_B		0x10000000
#define	SCR2_DSP_MODE_A		0x08000000
#define SCR2_REMOTE_INT		0x04000000
#define	SCR2_LOCAL_INT		0x01000000
#define	SCR2_DRAM_256K		0x00100000
#define SCR2_DRAM_1M		0x00010000
#define SCR2_TIMER_ON_IPL7	0x00008000
#define SCR2_ROM_WAITSTATES	0x00007000
#define SCR2_ROM_1M		0x00000800
#define SCR2_RTDATA		0x00000400
#define SCR2_RTCLK		0x00000200
#define SCR2_RTCE		0x00000100
#define	SCR2_ROM_OVERLAY	0x00000080
#define SCR2_DSP_IE		0x00000040
#define SCR2_MEM_EN		0x00000020
#define SCR2_LED		0x00000001

/* real time clock -- old is MC68HC68T1 chip, new is MCS1850 chip */
#define RTC_RAM         0x00            /* both */
#define RTC_SEC         0x20            /* old BCD coded date*/
#define RTC_MIN         0x21
#define RTC_HRS         0x22
#define RTC_DAY         0x23
#define RTC_DATE        0x24
#define RTC_MON         0x25
#define RTC_YR          0x26
#define RTC_ALARM_SEC   0x28
#define RTC_ALARM_MIN   0x29
#define RTC_ALARM_HR    0x2a

#define RTC_CNTR0       0x20            /* new */
#define RTC_CNTR1       0x21
#define RTC_CNTR2       0x22
#define RTC_CNTR3       0x23
#define RTC_ALARM0      0x24
#define RTC_ALARM1      0x25
#define RTC_ALARM2      0x26
#define RTC_ALARM3      0x27

#define RTC_STATUS      0x30            /* both */
#define RTC_CONTROL     0x31            /* both */
#define RTC_INTRCTL     0x32            /* old */

/* bits in RTC_STATUS */
#define RTC_NEW_CLOCK   0x80    /* new: set in new clock chip */
#define RTC_FTU         0x10    /* both: set when powered up but uninitialized */
#define RTC_INTR        0x08    /* new: interrupt asserted */
#define RTC_LOW_BATT    0x04    /* new: low battery */
#define RTC_ALARM       0x02    /* new: alarm interrupt */
#define RTC_RPD         0x01    /* new: request to power down */

/* bits in RTC_CONTROL */
#define RTC_START       0x80    /* both: start counters */
#define RTC_STOP        0x00    /* both: stop counters */
#define RTC_XTAL        0x30    /* old: xtal: line = 0, sel0 = sel1 = 1 */
#define RTC_AUTO_PON    0x20    /* new: auto poweron after power fail */
#define RTC_AE          0x10    /* new: alarm enable */
#define RTC_AC          0x08    /* new: alarm clear */
#define RTC_FTUC        0x04    /* new: first time up clear */
#define RTC_LBE         0x02    /* new: low battery enable */
#define RTC_RPDC        0x01    /* new: request to power down clear */

/* bits in RTC_INTRCTL */
#define RTC_PDOWN       0x40    /* both: power down, bit in RTC_CONTROL on new chip */
#define RTC_64HZ        0x06    /* old: periodic select = 64 Hz */
#define RTC_128HZ       0x05    /* old: periodic select = 128 Hz */
#define RTC_512HZ       0x03    /* old: periodic select = 512 Hz */

/* RTC address byte format */
#define RTC_WRITE       0x80
#define RTC_ADRS        0x3f


struct timer_reg {
  u_char msb;
  u_char lsb;
  u_char pad0;
  u_char pad1;
  u_char csr;
};

/* timer register */
#define TIMER_REG_ENABLE	0x80
#define TIMER_REG_UPDATE	0x40
#define TIMER_REG_MAX		0xffff		/* Maximum value of timer */
