/* $NetBSD: rtc.c,v 1.1.2.2 2002/06/23 17:40:24 jdolecek Exp $ */

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/clock_subr.h>

#include <machine/systemsw.h>

#include <mips/locore.h>
#include <mips/sibyte/dev/sbsmbusvar.h>

/* XXX should be in an x1241.h header */
#define	X1241REG_BL		0x10	/* block protect bits */
#define	X1241REG_SC		0x30	/* Seconds */
#define	X1241REG_MN		0x31	/* Minutes */
#define	X1241REG_HR		0x32	/* Hours */
#define	X1241REG_DT		0x33	/* Day of month */
#define	X1241REG_MO		0x34	/* Month */
#define	X1241REG_YR		0x35	/* Year */
#define	X1241REG_DW		0x36	/* Day of Week */
#define	X1241REG_Y2K		0x37	/* Year 2K */
#define	X1241REG_SR		0x3f	/* Status register */

/* Register bits for the status register */
#define	X1241REG_SR_BAT		0x80	/* currently on battery power */
#define	X1241REG_SR_RWEL	0x04	/* r/w latch is enabled, can write RTC */
#define	X1241REG_SR_WEL		0x02	/* r/w latch is unlocked, can enable r/w now */
#define	X1241REG_SR_RTCF	0x01	/* clock failed */

/* Register bits for the block protect register */
#define	X1241REG_BL_BP2		0x80	/* block protect 2 */
#define	X1241REG_BL_BP1		0x40	/* block protect 1 */
#define	X1241REG_BL_BP0		0x20	/* block protect 0 */
#define	X1241REG_BL_WD1		0x10
#define	X1241REG_BL_WD0		0x08

/* Register bits for the hours register */
#define	X1241REG_HR_MIL		0x80	/* military time format */

struct rtc_softc {
	struct device		sc_dev;
	int			sc_smbus_chan;
	int			sc_smbus_addr;
	struct todr_chip_handle	sc_ct;
};

static int rtc_match(struct device *, struct cfdata *, void *);
static void rtc_attach(struct device *, struct device *, void *);
static int rtc_gettime(todr_chip_handle_t, struct timeval *);
static int rtc_settime(todr_chip_handle_t, struct timeval *);
static int rtc_getcal(todr_chip_handle_t, int *);
static int rtc_setcal(todr_chip_handle_t, int);
static void rtc_inittodr(void *, time_t base);
static void rtc_resettodr(void *);
static void rtc_cal_timer(void);

static void time_smbus_init(int);
static int time_waitready(int);
static int time_readrtc(int, int, int);
static int time_writertc(int, int, int, int);

#define	WRITERTC(sc, dev, val)	\
	time_writertc((sc)->sc_smbus_chan, (sc)->sc_smbus_addr, (dev), (val))
#define	READRTC(sc, dev)	\
	time_readrtc((sc)->sc_smbus_chan, (sc)->sc_smbus_addr, (dev))


struct cfattach rtc_ca = {
	sizeof(struct rtc_softc), rtc_match, rtc_attach
};

static int rtcfound = 0;
struct rtc_softc *the_rtc;

static int
rtc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	
	return (!rtcfound);
}

static void
rtc_attach(struct device *parent, struct device *self, void *aux)
{
	struct smbus_attach_args *sa = aux;
	struct rtc_softc *sc = (void *)self;

	rtcfound = 1;
	the_rtc = sc;

	sc->sc_smbus_chan = sa->sa_interface;
	sc->sc_smbus_addr = sa->sa_device;

	/* Set up MI todr(9) stuff */
	sc->sc_ct.cookie = sc;
	sc->sc_ct.todr_settime = rtc_settime;
	sc->sc_ct.todr_gettime = rtc_gettime;
	sc->sc_ct.todr_getcal = rtc_getcal;
	sc->sc_ct.todr_setcal = rtc_setcal;

	system_set_todrfns(sc, rtc_inittodr, rtc_resettodr);

	printf("\n");
	rtc_cal_timer();	/* XXX */
}

static int
rtc_settime(todr_chip_handle_t handle, struct timeval *tv)
{
	struct rtc_softc *sc = handle->cookie;
	struct clock_ymdhms ymdhms;
	uint8_t year, y2k;

	clock_secs_to_ymdhms(tv->tv_sec, &ymdhms);

	time_smbus_init(sc->sc_smbus_chan);

	/* unlock writes to the CCR */
	WRITERTC(sc, X1241REG_SR, X1241REG_SR_WEL);
	WRITERTC(sc, X1241REG_SR, X1241REG_SR_WEL | X1241REG_SR_RWEL);

	/* set the time */

	WRITERTC(sc, X1241REG_HR, TOBCD(ymdhms.dt_hour) | X1241REG_HR_MIL);
	WRITERTC(sc, X1241REG_MN, TOBCD(ymdhms.dt_min));
	WRITERTC(sc, X1241REG_SC, TOBCD(ymdhms.dt_sec));

	/* set the date */

	y2k = (ymdhms.dt_year >= 2000) ? 0x20 : 0x19;
	year = ymdhms.dt_year % 100;

	WRITERTC(sc, X1241REG_MO, TOBCD(ymdhms.dt_mon));
	WRITERTC(sc, X1241REG_DT, TOBCD(ymdhms.dt_day));
	WRITERTC(sc, X1241REG_YR, TOBCD(year));
	WRITERTC(sc, X1241REG_Y2K, TOBCD(y2k));

	/* lock writes again */
	WRITERTC(sc, X1241REG_SR, 0);

	return (0);
}

static int
rtc_gettime(todr_chip_handle_t handle, struct timeval *tv)
{
	struct rtc_softc *sc = handle->cookie;
	struct clock_ymdhms ymdhms;
	uint8_t hour, year, y2k;
	uint8_t status;

	time_smbus_init(sc->sc_smbus_chan);
	ymdhms.dt_day = FROMBCD(READRTC(sc, X1241REG_DT));
	ymdhms.dt_mon =  FROMBCD(READRTC(sc, X1241REG_MO));
	year =  READRTC(sc, X1241REG_YR);
	y2k = READRTC(sc, X1241REG_Y2K);
	ymdhms.dt_year = FROMBCD(y2k) * 100 + FROMBCD(year);


	ymdhms.dt_sec = FROMBCD(READRTC(sc, X1241REG_SC));
	ymdhms.dt_min = FROMBCD(READRTC(sc, X1241REG_MN));
	hour = READRTC(sc, X1241REG_HR);
	ymdhms.dt_hour = FROMBCD(hour & ~X1241REG_HR_MIL);

	status = READRTC(sc, X1241REG_SR);

	if (status & X1241REG_SR_RTCF) {
		printf("%s: battery has failed, clock setting is not accurate\n",
		    sc->sc_dev.dv_xname);
		return (-1);
	}

	tv->tv_sec = clock_ymdhms_to_secs(&ymdhms);
	tv->tv_usec = 0;

	return (0);
}

static int
rtc_getcal(todr_chip_handle_t handle, int *vp)
{

	return EOPNOTSUPP;
}

static int
rtc_setcal(todr_chip_handle_t handle, int v)
{

	return EOPNOTSUPP;
}


static void
rtc_inittodr(void *cookie, time_t base)
{
	struct timeval todrtime;
	todr_chip_handle_t chip;
	struct rtc_softc *sc = cookie;
	int check;

	check = 0;
	if (sc == NULL) {
		printf("inittodr: rtc0 not present");
		time.tv_sec = base;
		time.tv_usec = 0;
		check = 1;
	} else {
		chip = &sc->sc_ct;
		if (todr_gettime(chip, &todrtime) != 0) {
			printf("inittodr: Error reading clock");
			time.tv_sec = base;
			time.tv_usec = 0;
			check = 1;
		} else {
			time = todrtime;
			if (time.tv_sec > base + 3 * SECDAY) {
				printf("inittodr: Clock has gained %ld days",
				    (time.tv_sec - base) / SECDAY);
				check = 1;
			} else if (time.tv_sec + SECDAY < base) {
				printf("inittodr: Clock has lost %ld day(s)",
				    (base - time.tv_sec) / SECDAY);
				check = 1;
			}
		}
	}
	if (check)
		printf(" - CHECK AND RESET THE DATE.\n");

}

static void
rtc_resettodr(void *cookie)
{
	struct rtc_softc *sc = cookie;

	if (time.tv_sec == 0)
		return;

	if (todr_settime(&sc->sc_ct, (struct timeval *)&time) != 0)
		printf("resettodr: cannot set time in time-of-day clock\n");
}

#define	NITERS			3
#define	RTC_SECONDS(rtc)	FROMBCD(READRTC((rtc), X1241REG_SC))

/*
 * Since it takes so long to read the complete time/date values from
 * the RTC over the SMBus, we only read the seconds value.
 * Later versions of the SWARM will hopefully have the RTC interrupt
 * attached so we can do the clock calibration much more quickly and
 * with a higher resolution.
 */
static void
rtc_cal_timer(void)
{
	uint32_t ctrdiff[NITERS], startctr, endctr;
	int sec, lastsec, i;

	if (rtcfound == 0) {
		printf("rtc_cal_timer before rtc attached\n");
		return;
	}

	printf("%s: calibrating CPU clock", the_rtc->sc_dev.dv_xname);

	/*
	 * Run the loop an extra time to wait for the second to tick over
	 * and to prime the cache.
	 */
	time_smbus_init(the_rtc->sc_smbus_chan);
	sec = RTC_SECONDS(the_rtc);
	endctr = mips3_cp0_count_read();

	for (i = 0; i < NITERS; i++) {
		int diff;

 again:
		lastsec = sec;
		startctr = endctr;
		
		/* Wait for the timer to tick over. */
		do {
			// time_smbus_init(the_rtc->sc_smbus_chan);
			sec = RTC_SECONDS(the_rtc);
		} while (lastsec == sec);
		endctr = mips3_cp0_count_read();

		diff = sec - lastsec;
		if (diff < 0)
			diff += 60;

		/* Sometimes we appear to skip a second.  Clock jitter? */
		if (diff > 1)
			goto again;

		if (endctr < startctr)
			ctrdiff[i] = 0xffffffff - startctr + endctr;
		else
			ctrdiff[i] = endctr - startctr;
	}
	printf("\n");

	/* Compute the number of cycles per second. */
	curcpu()->ci_cpu_freq = ((ctrdiff[1] + ctrdiff[2]) / 2);

	/* Compute the delay divisor. */
	curcpu()->ci_divisor_delay = curcpu()->ci_cpu_freq / 1000000;

	/* Compute clock cycles per hz */
	curcpu()->ci_cycles_per_hz = curcpu()->ci_cpu_freq / hz;

	printf("%s: timer calibration: %lu cycles/sec [(%u, %u)]\n",
	    the_rtc->sc_dev.dv_xname, curcpu()->ci_cpu_freq,
	    ctrdiff[1], ctrdiff[2]);
}
#undef RTC_SECONDS

/* XXX eville direct-access-to-the-device code follows... */

/*  *********************************************************************
    *
    *  Copyright 2000,2001
    *  Broadcom Corporation. All rights reserved.
    *  
    *  This software is furnished under license and may be used and 
    *  copied only in accordance with the following terms and 
    *  conditions.  Subject to these conditions, you may download, 
    *  copy, install, use, modify and distribute modified or unmodified 
    *  copies of this software in source and/or binary form.  No title 
    *  or ownership is transferred hereby.
    *  
    *  1) Any source code used, modified or distributed must reproduce 
    *     and retain this copyright notice and list of conditions as 
    *     they appear in the source file.
    *  
    *  2) No right is granted to use any trade name, trademark, or 
    *     logo of Broadcom Corporation. Neither the "Broadcom 
    *     Corporation" name nor any trademark or logo of Broadcom 
    *     Corporation may be used to endorse or promote products 
    *     derived from this software without the prior written 
    *     permission of Broadcom Corporation.
    *  
    *  3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR
    *     IMPLIED WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED 
    *     WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
    *     PURPOSE, OR NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT 
    *     SHALL BROADCOM BE LIABLE FOR ANY DAMAGES WHATSOEVER, AND IN 
    *     PARTICULAR, BROADCOM SHALL NOT BE LIABLE FOR DIRECT, INDIRECT, 
    *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
    *     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
    *     GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
    *     BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
    *     OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
    *     TORT (INCLUDING NEGLIGENCE OR OTHERWISE), EVEN IF ADVISED OF 
    *     THE POSSIBILITY OF SUCH DAMAGE.
    ********************************************************************* */

#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_smbus.h>

#define	READ_REG(rp)		(mips3_ld((uint64_t *)(MIPS_PHYS_TO_KSEG1(rp))))
#define	WRITE_REG(rp, val)	(mips3_sd((uint64_t *)(MIPS_PHYS_TO_KSEG1(rp)), (val)))

static void
time_smbus_init(int chan)
{
	uint32_t reg;

	reg = A_SMB_REGISTER(chan, R_SMB_FREQ);
	WRITE_REG(reg, K_SMB_FREQ_100KHZ);
	reg = A_SMB_REGISTER(chan, R_SMB_CONTROL);
	WRITE_REG(reg, 0);	/* not in direct mode, no interrupts, will poll */
}

static int
time_waitready(int chan)
{
	uint32_t reg;
	uint64_t status;

	reg = A_SMB_REGISTER(chan, R_SMB_STATUS);

	for (;;) {
		status = READ_REG(reg);
		if (status & M_SMB_BUSY)
			continue;
		break;
	}

	if (status & M_SMB_ERROR) {
		WRITE_REG(reg, (status & M_SMB_ERROR));
		return -1;
	}
	return 0;
}

static int
time_readrtc(int chan, int slaveaddr, int devaddr)
{
	uint32_t reg;
	int err;

	/*
	 * Make sure the bus is idle (probably should
	 * ignore error here)
	 */

	if (time_waitready(chan) < 0)
		return -1;

	/*
	 * Write the device address to the controller. There are two
	 * parts, the high part goes in the "CMD" field, and the 
	 * low part is the data field.
	 */

	reg = A_SMB_REGISTER(chan, R_SMB_CMD);
	WRITE_REG(reg, ((devaddr >> 8) & 0x7));

	/*
	 * Write the data to the controller
	 */

	reg = A_SMB_REGISTER(chan, R_SMB_DATA);
	WRITE_REG(reg, ((devaddr & 0xff) & 0xff));

	/*
	 * Start the command
	 */

	reg = A_SMB_REGISTER(chan, R_SMB_START);
	WRITE_REG(reg, V_SMB_TT(K_SMB_TT_WR2BYTE) | slaveaddr);

	/*
	 * Wait till done
	 */

	err = time_waitready(chan);
	if (err < 0)
		return err;

	/*
	 * Read the data byte
	 */

	WRITE_REG(reg, V_SMB_TT(K_SMB_TT_RD1BYTE) | slaveaddr);

	err = time_waitready(chan);
	if (err < 0)
		return err;

	reg = MIPS_PHYS_TO_KSEG1(A_SMB_REGISTER(chan, R_SMB_DATA));
	err = READ_REG(reg);

	return (err & 0xff);
}

static int
time_writertc(int chan, int slaveaddr, int devaddr, int b)
{
	uint32_t reg;
	int err;
	int64_t timer;

	/*
	 * Make sure the bus is idle (probably should
	 * ignore error here)
	 */

	if (time_waitready(chan) < 0)
		return -1;

	/*
	 * Write the device address to the controller. There are two
	 * parts, the high part goes in the "CMD" field, and the 
	 * low part is the data field.
	 */

	reg = A_SMB_REGISTER(chan, R_SMB_CMD);
	WRITE_REG(reg, ((devaddr >> 8) & 0x7));

	/*
	 * Write the data to the controller
	 */

	reg = A_SMB_REGISTER(chan, R_SMB_DATA);
	WRITE_REG(reg, ((devaddr & 0xFF) | ((b & 0xFF) << 8)));

	/*
	 * Start the command.  Keep pounding on the device until it
	 * submits or the timer expires, whichever comes first.  The
	 * datasheet says writes can take up to 10ms, so we'll give it 500.
	 */

	reg = A_SMB_REGISTER(chan, R_SMB_START);
	WRITE_REG(reg, V_SMB_TT(K_SMB_TT_WR3BYTE) | slaveaddr);

	/*
	 * Wait till the SMBus interface is done
	 */ 

	err = time_waitready(chan);
	if (err < 0)
		return err;

	/*
	 * Pound on the device with a current address read
	 * to poll for the write complete
	 */

	err = -1; 
	timer = 100000000;	/* XXX */

	while (timer-- > 0) {

		WRITE_REG(reg, V_SMB_TT(K_SMB_TT_RD1BYTE) | slaveaddr);

		err = time_waitready(chan);
		if (err == 0)
			break;
	}

	return err;
}
