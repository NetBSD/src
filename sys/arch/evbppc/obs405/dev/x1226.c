/*	$NetBSD: x1226.c,v 1.1 2003/09/23 14:45:15 shige Exp $	*/

/*
 * Copyright (c) 2003 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUASLITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: x1226.c,v 1.1 2003/09/23 14:45:15 shige Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/rtc.h>
#include <machine/bus.h>

#include <powerpc/ibm4xx/dev/iicvar.h>
#include <powerpc/ibm4xx/dev/todclockvar.h>
#include <evbppc/obs405/dev/x1226reg.h>

struct xrtc_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct iic_softc	*adap;
};

static void	xrtc_attach(struct device *, struct device *, void *);
static int	xrtc_match(struct device *, struct cfdata *, void *);

static int	xrtc_read(void *, rtc_t *);
static int	xrtc_write(void *, rtc_t *);

static inline u_char	x1226_read(struct xrtc_softc *, u_int16_t);
static inline u_char	x1226_write(struct xrtc_softc *, u_int16_t, u_char);
static int		x1226_lock(struct xrtc_softc *);
static int		x1226_unlock(struct xrtc_softc *);

/* device and attach structures */
CFATTACH_DECL(rtc, sizeof(struct xrtc_softc),
    xrtc_match, xrtc_attach, NULL, NULL);

/*
 * xrtc_match()
 */

static int
xrtc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct iicbus_attach_args *iaa = aux;

	/* match only RTC devices */
	if (strcmp(iaa->iicb_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

/*
 * xrtc_attach()
 */

static void
xrtc_attach(struct device *parent, struct device *self, void *aux)
{
	struct xrtc_softc *sc = (struct xrtc_softc *)self;
	struct iicbus_attach_args *iaa = aux;
	struct todclock_attach_args ta;

	sc->sc_iot = 0;
	sc->sc_ioh = iaa->iicb_addr;
	sc->adap = iaa->adap;

	/* Make sure the clock is running */
	/* x1226_unlock(sc); */
	if (x1226_read(sc, X1226_SR) & X1226_SR_RTCF)
		printf(": power failure, RTC unreliable");
	printf("\n");

#ifdef DEBUG
	{
		rtc_t rtc;
		xrtc_read(sc, &rtc);
		printf("RTC: %02d%02d/%02d/%02d %02d:%02d:%02d\n",
			rtc.rtc_cen, rtc.rtc_year,
			rtc.rtc_mon, rtc.rtc_day,
			rtc.rtc_hour, rtc.rtc_min, rtc.rtc_sec);
	}
#endif

	ta.ta_name = "todclock";
	ta.ta_rtc_arg = sc;
	ta.ta_rtc_write = xrtc_write; 
	ta.ta_rtc_read = xrtc_read;
	ta.ta_flags = 0;
	config_found(self, &ta, NULL);
}

static inline u_char
x1226_read(struct xrtc_softc *sc, u_int16_t addr)
{
	u_char ret, baddr[2];
	int addrmsk = (X1226_SIZE - 1);

	baddr[0] = (((addr & addrmsk) >> 8) & 0xff);
	baddr[1] = ((addr & addrmsk) & 0xff);

	iic_read(sc->adap, X1226_DEVID_CCR, baddr, 2, &ret, 1);

	return ret;
}

static inline u_char
x1226_write(struct xrtc_softc *sc, u_int16_t addr, u_char d)
{
	u_char ret, data[1], baddr[2];
	int addrmsk = (X1226_SIZE - 1);

	data[0] = d;
	baddr[0] = (((addr & addrmsk) >> 8) & 0xff);
	baddr[1] = ((addr & addrmsk) & 0xff);

	ret = iic_write(sc->adap, X1226_DEVID_CCR, baddr, 2, data, 1);

	return ret;
}

#define BIN2BCD(x)	((((x) / 10) << 4) | (x % 10))
#define BCD2BIN(x)	(((((x) >> 4) & 0xf) * 10) + ((x) & 0xf))

static int
x1226_unlock(struct xrtc_softc *sc)
{
	u_char sr;

 	sr = X1226_SR_WEL;
	x1226_write(sc, X1226_SR, sr);
 	sr |= X1226_SR_RWEL;
	x1226_write(sc, X1226_SR, sr);

	sr = 0;
	sr = x1226_read(sc, X1226_SR);
	sr &= (X1226_SR_RWEL | X1226_SR_WEL);
	if (sr != (X1226_SR_RWEL | X1226_SR_WEL)) {
		return 1;
	}
	return 0;
}

static int
x1226_lock(struct xrtc_softc *sc)
{
	u_char sr;

	sr = 0;
	x1226_write(sc, X1226_SR, sr);

	sr = 0;
	sr = x1226_read(sc, X1226_SR);
	sr &= (X1226_SR_RWEL | X1226_SR_WEL);
	if (sr != 0) {
		return 1;
	}
	return 0;
}

static int
xrtc_write(void * arg, rtc_t * rtc)
{
	struct xrtc_softc *sc = arg;

	if (x1226_unlock(sc) != 0) {
		return (1);
	}
	
	x1226_write(sc, X1226_RTC_SC,
		BIN2BCD(rtc->rtc_sec)   & 0x7f);
	x1226_write(sc, X1226_RTC_MN,
		BIN2BCD(rtc->rtc_min)   & 0x7f);
	x1226_write(sc, X1226_RTC_HR,			/* 24 hour format */
		(BIN2BCD(rtc->rtc_hour) & 0x1f) | X1226_RTC_HR_MIL);
	x1226_write(sc, X1226_RTC_DT,
		BIN2BCD(rtc->rtc_day)   & 0x3f);
	x1226_write(sc, X1226_RTC_MO,
		BIN2BCD(rtc->rtc_mon)   & 0x1f);
	x1226_write(sc, X1226_RTC_YR,
		BIN2BCD(rtc->rtc_year)  & 0xff);
	x1226_write(sc, X1226_RTC_Y2K,
		BIN2BCD(rtc->rtc_cen)   & 0x3f);

	x1226_lock(sc);

	xrtc_read(arg, rtc);

	return (0);
}

static int
xrtc_read(void *arg, rtc_t *rtc)
{
	struct xrtc_softc *sc = arg;
	u_char hreg;
	
	rtc->rtc_micro = 0;
	rtc->rtc_centi = 0;

	/* YYYY */
	rtc->rtc_cen = BCD2BIN(x1226_read(sc, X1226_RTC_Y2K) & 0x3f);
	rtc->rtc_year = BCD2BIN(x1226_read(sc, X1226_RTC_YR));

	/* MM */
	rtc->rtc_mon = BCD2BIN(x1226_read(sc, X1226_RTC_MO)  & 0x1f);

	/* DD */
	rtc->rtc_day = BCD2BIN(x1226_read(sc, X1226_RTC_DT)  & 0x3f);

	/* HH */
	hreg = x1226_read(sc, X1226_RTC_HR);
	rtc->rtc_hour = BCD2BIN(hreg & 0x1f);
	if (!(hreg & X1226_RTC_HR_MIL)) {
		if (hreg & X1226_RTC_HR_H21) {
			rtc->rtc_hour += 12;
			if (rtc->rtc_hour >= 24)
				rtc->rtc_hour = 0;
		}
	}

	/* MM */
	rtc->rtc_min   = BCD2BIN(x1226_read(sc, X1226_RTC_MN)  & 0x7f);

	/* SS */
	rtc->rtc_sec   = BCD2BIN(x1226_read(sc, X1226_RTC_SC)  & 0x7f);

	return (1);
}
