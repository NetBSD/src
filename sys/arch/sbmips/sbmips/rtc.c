/* $NetBSD: rtc.c,v 1.16.38.2 2010/01/12 18:21:17 matt Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtc.c,v 1.16.38.2 2010/01/12 18:21:17 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <dev/clock_subr.h>

#include <machine/swarm.h>
#include <machine/systemsw.h>

#include <mips/locore.h>
#include <mips/sibyte/dev/sbsmbusvar.h>

#include <dev/smbus/m41t81reg.h>
#include <dev/smbus/x1241reg.h>

struct rtc_softc {
	device_t		sc_dev;
	int			sc_smbus_chan;
	int			sc_smbus_addr;
	int			sc_type;
	struct todr_chip_handle	sc_ct;
};

/* "types" for RTCs we support */
#define	SMB_1BYTE_ADDR	1
#define	SMB_2BYTE_ADDR	2

static int xirtc_match(device_t, cfdata_t , void *);
static void xirtc_attach(device_t, device_t, void *);
static int xirtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int xirtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

static int strtc_match(device_t, cfdata_t , void *);
static void strtc_attach(device_t, device_t, void *);
static int strtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int strtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

static void rtc_cal_timer(void);

static void time_smbus_init(int);
static int time_waitready(int);
static int time_readrtc(int, int, int, int);
static int time_writertc(int, int, int, int, int);

#define	WRITERTC(sc, dev, val)	\
	time_writertc((sc)->sc_smbus_chan, (sc)->sc_smbus_addr, (dev), (sc)->sc_type, (val))
#define	READRTC(sc, dev)	\
	time_readrtc((sc)->sc_smbus_chan, (sc)->sc_smbus_addr, (dev), (sc)->sc_type)


CFATTACH_DECL_NEW(xirtc, sizeof(struct rtc_softc),
    xirtc_match, xirtc_attach, NULL, NULL);

CFATTACH_DECL_NEW(m41t81rtc, sizeof(struct rtc_softc),
    strtc_match, strtc_attach, NULL, NULL);

static int rtcfound = 0;
struct rtc_softc *the_rtc;

/*
 * Xicor X1241 RTC support.
 */
static int
xirtc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct smbus_attach_args *sa = aux;
	int ret;

	time_smbus_init(sa->sa_interface);

	if ((sa->sa_interface != X1241_SMBUS_CHAN) ||
	    (sa->sa_device != X1241_RTC_SLAVEADDR))
		return (0);
	
	ret = time_readrtc(sa->sa_interface, sa->sa_device, SMB_2BYTE_ADDR, X1241REG_SC);
	if (ret < 0)
		return (0);

	return (!rtcfound);
}

static void
xirtc_attach(device_t parent, device_t self, void *aux)
{
	struct smbus_attach_args *sa = aux;
	struct rtc_softc *sc = device_private(self);

	rtcfound = 1;
	the_rtc = sc;

	sc->sc_dev = self;
	sc->sc_smbus_chan = sa->sa_interface;
	sc->sc_smbus_addr = sa->sa_device;
	sc->sc_type = SMB_2BYTE_ADDR;	/* Two-byte register addresses on the Xicor */


	/* Set up MI todr(9) stuff */
	sc->sc_ct.cookie = sc;
	sc->sc_ct.todr_settime_ymdhms = xirtc_settime;
	sc->sc_ct.todr_gettime_ymdhms = xirtc_gettime;

	todr_attach(&sc->sc_ct);

	aprint_normal("\n");
	rtc_cal_timer();	/* XXX */
}

static int
xirtc_settime(todr_chip_handle_t handle, struct clock_ymdhms *ymdhms)
{
	struct rtc_softc *sc = handle->cookie;
	uint8_t year, y2k;

	time_smbus_init(sc->sc_smbus_chan);

	/* unlock writes to the CCR */
	WRITERTC(sc, X1241REG_SR, X1241REG_SR_WEL);
	WRITERTC(sc, X1241REG_SR, X1241REG_SR_WEL | X1241REG_SR_RWEL);

	/* set the time */
	WRITERTC(sc, X1241REG_HR, TOBCD(ymdhms->dt_hour) | X1241REG_HR_MIL);
	WRITERTC(sc, X1241REG_MN, TOBCD(ymdhms->dt_min));
	WRITERTC(sc, X1241REG_SC, TOBCD(ymdhms->dt_sec));

	/* set the date */
	y2k = (ymdhms->dt_year >= 2000) ? 0x20 : 0x19;
	year = ymdhms->dt_year % 100;

	WRITERTC(sc, X1241REG_MO, TOBCD(ymdhms->dt_mon));
	WRITERTC(sc, X1241REG_DT, TOBCD(ymdhms->dt_day));
	WRITERTC(sc, X1241REG_YR, TOBCD(year));
	WRITERTC(sc, X1241REG_Y2K, TOBCD(y2k));

	/* lock writes again */
	WRITERTC(sc, X1241REG_SR, 0);

	return (0);
}

static int
xirtc_gettime(todr_chip_handle_t handle, struct clock_ymdhms *ymdhms)
{
	struct rtc_softc *sc = handle->cookie;
	uint8_t hour, year, y2k;
	uint8_t status;

	time_smbus_init(sc->sc_smbus_chan);
	ymdhms->dt_day = FROMBCD(READRTC(sc, X1241REG_DT));
	ymdhms->dt_mon =  FROMBCD(READRTC(sc, X1241REG_MO));
	year =  READRTC(sc, X1241REG_YR);
	y2k = READRTC(sc, X1241REG_Y2K);
	ymdhms->dt_year = FROMBCD(y2k) * 100 + FROMBCD(year);


	ymdhms->dt_sec = FROMBCD(READRTC(sc, X1241REG_SC));
	ymdhms->dt_min = FROMBCD(READRTC(sc, X1241REG_MN));
	hour = READRTC(sc, X1241REG_HR);
	ymdhms->dt_hour = FROMBCD(hour & ~X1241REG_HR_MIL);

	status = READRTC(sc, X1241REG_SR);

	if (status & X1241REG_SR_RTCF) {
		printf("%s: battery has failed, clock setting is not accurate\n",
		    device_xname(sc->sc_dev));
		return (EIO);
	}

	return (0);
}

/*
 * ST M41T81 RTC support.
 */
static int
strtc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct smbus_attach_args *sa = aux;
	int ret;

	if ((sa->sa_interface != M41T81_SMBUS_CHAN) ||
	    (sa->sa_device != M41T81_SLAVEADDR))
		return (0);

	time_smbus_init(sa->sa_interface);
	
	ret = time_readrtc(sa->sa_interface, sa->sa_device, SMB_1BYTE_ADDR, M41T81_SEC);
	if (ret < 0)
		return (0);

	return (!rtcfound);
}

static void
strtc_attach(device_t parent, device_t self, void *aux)
{
	struct smbus_attach_args *sa = aux;
	struct rtc_softc *sc = device_private(self);

	rtcfound = 1;
	the_rtc = sc;

	sc->sc_dev = self;
	sc->sc_smbus_chan = sa->sa_interface;
	sc->sc_smbus_addr = sa->sa_device;
	sc->sc_type = SMB_1BYTE_ADDR;	/* One-byte register addresses on the ST */

	/* Set up MI todr(9) stuff */
	sc->sc_ct.cookie = sc;
	sc->sc_ct.todr_settime_ymdhms = strtc_settime;
	sc->sc_ct.todr_gettime_ymdhms = strtc_gettime;

	todr_attach(&sc->sc_ct);

	aprint_normal("\n");
	rtc_cal_timer();	/* XXX */
}

static int
strtc_settime(todr_chip_handle_t handle, struct clock_ymdhms *ymdhms)
{
	struct rtc_softc *sc = handle->cookie;
	uint8_t hour;

	time_smbus_init(sc->sc_smbus_chan);

	hour = TOBCD(ymdhms->dt_hour);
	if (ymdhms->dt_year >= 2000)	/* Should be always true! */
		hour |= M41T81_HOUR_CB | M41T81_HOUR_CEB;

	/* set the time */
	WRITERTC(sc, M41T81_SEC, TOBCD(ymdhms->dt_sec));
	WRITERTC(sc, M41T81_MIN, TOBCD(ymdhms->dt_min));
	WRITERTC(sc, M41T81_HOUR, hour);

	/* set the date */
	WRITERTC(sc, M41T81_DATE, TOBCD(ymdhms->dt_day));
	WRITERTC(sc, M41T81_MON, TOBCD(ymdhms->dt_mon));
	WRITERTC(sc, M41T81_YEAR, TOBCD(ymdhms->dt_year % 100));

	return (0);
}

static int
strtc_gettime(todr_chip_handle_t handle, struct clock_ymdhms *ymdhms)
{
	struct rtc_softc *sc = handle->cookie;
	uint8_t hour;

	time_smbus_init(sc->sc_smbus_chan);

	ymdhms->dt_sec = FROMBCD(READRTC(sc, M41T81_SEC));
	ymdhms->dt_min = FROMBCD(READRTC(sc, M41T81_MIN));
	hour = READRTC(sc, M41T81_HOUR & M41T81_HOUR_MASK);
	ymdhms->dt_hour = FROMBCD(hour & M41T81_HOUR_MASK);

	ymdhms->dt_day = FROMBCD(READRTC(sc, M41T81_DATE));
	ymdhms->dt_mon =  FROMBCD(READRTC(sc, M41T81_MON));
	ymdhms->dt_year =  1900 + FROMBCD(READRTC(sc, M41T81_YEAR));
	if (hour & M41T81_HOUR_CB)
		ymdhms->dt_year += 100;

	return (0);
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
return;	/* XXX XXX */

	printf("%s: calibrating CPU clock", device_xname(the_rtc->sc_dev));

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
	    device_xname(the_rtc->sc_dev), curcpu()->ci_cpu_freq,
	    ctrdiff[1], ctrdiff[2]);
}
#undef RTC_SECONDS

/* XXX eville direct-access-to-the-device code follows... */

/*
 * Copyright 2000,2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_smbus.h>

#define	READ_REG(rp)		(mips3_ld((volatile uint64_t *)(MIPS_PHYS_TO_KSEG1(rp))))
#define	WRITE_REG(rp, val)	(mips3_sd((volatile uint64_t *)(MIPS_PHYS_TO_KSEG1(rp)), (val)))

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
		return (-1);
	}
	return (0);
}

static int
time_readrtc(int chan, int slaveaddr, int devaddr, int type)
{
	uint32_t reg;
	int err;

	/*
	 * Make sure the bus is idle (probably should
	 * ignore error here)
	 */

	if (time_waitready(chan) < 0)
		return (-1);

	if (type == SMB_2BYTE_ADDR) {
		/*
		 * Write the device address to the controller. There are two
		 * parts, the high part goes in the "CMD" field, and the 
		 * low part is the data field.
		 */

		reg = A_SMB_REGISTER(chan, R_SMB_CMD);
		WRITE_REG(reg, (devaddr >> 8) & 0x7);

		/*
		 * Write the data to the controller
		 */

		reg = A_SMB_REGISTER(chan, R_SMB_DATA);
		WRITE_REG(reg, (devaddr & 0xff) & 0xff);
	} else { /* SMB_1BYTE_ADDR */
		/*
		 * Write the device address to the controller.
		 */

		reg = A_SMB_REGISTER(chan, R_SMB_CMD);
		WRITE_REG(reg, devaddr & 0xff);
	}

	/*
	 * Start the command
	 */

	reg = A_SMB_REGISTER(chan, R_SMB_START);
	if (type == SMB_2BYTE_ADDR)
		WRITE_REG(reg, V_SMB_TT(K_SMB_TT_WR2BYTE) | V_SMB_ADDR(slaveaddr));
	else /* SMB_1BYTE_ADDR */
		WRITE_REG(reg, V_SMB_TT(K_SMB_TT_WR1BYTE) | V_SMB_ADDR(slaveaddr));

	/*
	 * Wait till done
	 */

	err = time_waitready(chan);
	if (err < 0)
		return (err);

	/*
	 * Read the data byte
	 */

	WRITE_REG(reg, V_SMB_TT(K_SMB_TT_RD1BYTE) | V_SMB_ADDR(slaveaddr));

	err = time_waitready(chan);
	if (err < 0)
		return (err);

	reg = A_SMB_REGISTER(chan, R_SMB_DATA);
	err = READ_REG(reg);

	return (err & 0xff);
}

static int
time_writertc(int chan, int slaveaddr, int devaddr, int type, int b)
{
	uint32_t reg;
	int err, timer;

	/*
	 * Make sure the bus is idle (probably should
	 * ignore error here)
	 */

	if (time_waitready(chan) < 0)
		return (-1);

	/*
	 * Write the device address to the controller. There are two
	 * parts, the high part goes in the "CMD" field, and the 
	 * low part is the data field.
	 */

	reg = A_SMB_REGISTER(chan, R_SMB_CMD);
	if (type == SMB_2BYTE_ADDR)
		WRITE_REG(reg, (devaddr >> 8) & 0x7);
	else /* SMB_1BYTE_ADDR */
		WRITE_REG(reg, devaddr & 0xff);

	/*
	 * Write the data to the controller
	 */

	reg = A_SMB_REGISTER(chan, R_SMB_DATA);
	if (type == SMB_2BYTE_ADDR)
		WRITE_REG(reg, (devaddr & 0xff) | ((b & 0xff) << 8));
	else /* SMB_1BYTE_ADDR */
		WRITE_REG(reg, b & 0xff);

	/*
	 * Start the command.  Keep pounding on the device until it
	 * submits or the timer expires, whichever comes first.  The
	 * datasheet says writes can take up to 10ms, so we'll give it 500.
	 */

	reg = A_SMB_REGISTER(chan, R_SMB_START);
	if (type == SMB_2BYTE_ADDR)
		WRITE_REG(reg, V_SMB_TT(K_SMB_TT_WR3BYTE) | V_SMB_ADDR(slaveaddr));
	else /* SMB_1BYTE_ADDR */
		WRITE_REG(reg, V_SMB_TT(K_SMB_TT_WR2BYTE) | V_SMB_ADDR(slaveaddr));

	/*
	 * Wait till the SMBus interface is done
	 */ 

	err = time_waitready(chan);
	if (err < 0)
		return (err);

	/*
	 * Pound on the device with a current address read
	 * to poll for the write complete
	 */

	err = -1; 
	timer = 100000000;	/* XXX */

	while (timer-- > 0) {
		WRITE_REG(reg, V_SMB_TT(K_SMB_TT_RD1BYTE) | V_SMB_ADDR(slaveaddr));

		err = time_waitready(chan);
		if (err == 0)
			break;
	}

	return (err);
}
