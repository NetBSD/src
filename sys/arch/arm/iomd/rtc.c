/*	$NetBSD: rtc.c,v 1.1.4.2 2002/02/28 04:07:38 nathanw Exp $	*/

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * rtc.c
 *
 * Routines to read and write the RTC and CMOS RAM
 *
 * Created      : 13/10/94
 * Updated	: 15/07/2000	DD
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <machine/conf.h>
#include <machine/rtc.h>
#include <arm/iomd/iic.h>
#include <arm/iomd/todclockvar.h>

struct rtc_softc {
	struct device	sc_dev;
	int		sc_flags;
#define RTC_BROKEN	1
#define RTC_OPEN	2
};

void rtcattach __P((struct device *parent, struct device *self, void *aux));
int rtcmatch __P((struct device *parent, struct cfdata *cf, void *aux));
int rtc_read __P((void *, rtc_t *));
int rtc_write __P((void *, rtc_t *));

static __inline int hexdectodec __P((u_char));
static __inline int dectohexdec __P((u_char));
static int twodigits __P((char *, int));

/* Read a byte from CMOS RAM */

int
cmos_read(location)
	int location;
{
	u_char buff;

/*
 * This commented code dates from when I was translating CMOS address
 * from the RISCOS addresses. Now all addresses are specifed as
 * actual addresses in the CMOS RAM
 */

/* 
 	if (location > 0xF0)
		return(-1);

	if (location < 0xC0)
		buff = location + 0x40;
	else
		buff = location - 0xB0;
*/
	buff = location;

	if (iic_control(RTC_Write, &buff, 1))
		return(-1);
	if (iic_control(RTC_Read, &buff, 1))
		return(-1);

	return(buff);
}


/* Write a byte to CMOS RAM */

int
cmos_write(location, value)
	int location;
	int value;
{
	u_char buff[2];
	int oldvalue, oldsum;

/*
 * This commented code dates from when I was translating CMOS address
 * from the RISCOS addresses. Now all addresses are specifed as
 * actual addresses in the CMOS RAM
 */

/*	if (location > 0xF0)
		return(-1);

	if (location < 0xC0)
		buff = location + 0x40;
	else
		buff = location - 0xB0;
*/
	buff[0] = location;
	buff[1] = value;

/* Read the old CMOS location value and old checksum */
	oldvalue = cmos_read(location);
	if (oldvalue<0)
		return(-1);
	oldsum = cmos_read(RTC_ADDR_CHECKSUM);
	if (oldsum<0)
		return(-1);
		
	if (iic_control(RTC_Write, buff, 2))
		return(-1);

/* Now update the checksum. This code only modifies the value. It does */
/* not recalculate it */

	buff[0] = RTC_ADDR_CHECKSUM;
	buff[1] = oldsum - oldvalue + value;
	if (iic_control(RTC_Write, buff, 2))
		return(-1);
	
	return(0);
}


/* Hex to BCD and BCD to hex conversion routines */

static __inline int
hexdectodec(n)
	u_char n;
{
	return(((n >> 4) & 0x0F) * 10 + (n & 0x0F));
}

static __inline int
dectohexdec(n)
	u_char n;
{
	return(((n / 10) << 4) + (n % 10));
}


/* Write the RTC data from an 8 byte buffer */

int
rtc_write(arg, rtc)
	void *arg;
	rtc_t *rtc;
{
	u_char buff[8];

	buff[0] = 1;

	buff[1] = dectohexdec(rtc->rtc_centi);
	buff[2] = dectohexdec(rtc->rtc_sec);
	buff[3] = dectohexdec(rtc->rtc_min);
	buff[4] = dectohexdec(rtc->rtc_hour) & 0x3f;
	buff[5] = dectohexdec(rtc->rtc_day) | ((rtc->rtc_year & 3) << 6);
	buff[6] = dectohexdec(rtc->rtc_mon);

	if (iic_control(RTC_Write, buff, 7))
		return(0);

	if (cmos_write(RTC_ADDR_YEAR, rtc->rtc_year))
		return(0);
	if (cmos_write(RTC_ADDR_CENT, rtc->rtc_cen))
		return(0);

	return(1);
}


/* Read the RTC data into a 8 byte buffer */

int
rtc_read(arg, rtc)
	void *arg;
	rtc_t *rtc;
{
	u_char buff[8];
	int byte;
    
	buff[0] = 0;

	if (iic_control(RTC_Write, buff, 1))
		return(0);

	if (iic_control(RTC_Read, buff, 8))
		return(0);

	rtc->rtc_micro = 0;
	rtc->rtc_centi = hexdectodec(buff[1] & 0xff);
	rtc->rtc_sec   = hexdectodec(buff[2] & 0x7f);
	rtc->rtc_min   = hexdectodec(buff[3] & 0x7f);
	rtc->rtc_hour  = hexdectodec(buff[4] & 0x3f);

	/* If in 12 hour mode need to look at the AM/PM flag */
	
	if (buff[4] & 0x80)
		rtc->rtc_hour += (buff[4] & 0x40) ? 12 : 0;

	rtc->rtc_day   = hexdectodec(buff[5] & 0x3f);
	rtc->rtc_mon   = hexdectodec(buff[6] & 0x1f);

	byte = cmos_read(RTC_ADDR_YEAR);
	if (byte == -1)
		return(0);
	rtc->rtc_year = byte; 

	byte = cmos_read(RTC_ADDR_CENT);
	if (byte == -1)
		return(0);
	rtc->rtc_cen = byte; 

	return(1);
}

/* device and attach structures */

struct cfattach rtc_ca = {
	sizeof(struct rtc_softc), rtcmatch, rtcattach
};

extern struct cfdriver rtc_cd;

/*
 * rtcmatch()
 *
 * Validate the IIC address to make sure its an RTC we understand
 */

int
rtcmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct iicbus_attach_args *ib = aux;

	if ((ib->ib_addr & IIC_PCF8583_MASK) == IIC_PCF8583_ADDR)
		return(1);

	return(0);
}

/*
 * rtcattach()
 *
 * Attach the rtc device
 */

void
rtcattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct rtc_softc *sc = (struct rtc_softc *)self;
	struct iicbus_attach_args *ib = aux;
	u_char buff[1];
	struct todclock_attach_args ta;

	sc->sc_flags |= RTC_BROKEN;
	if ((ib->ib_addr & IIC_PCF8583_MASK) == IIC_PCF8583_ADDR) {
		printf(": PCF8583");

		/* Read RTC register 0 and report info found */

		buff[0] = 0;

		if (iic_control(RTC_Write, buff, 1))
			return;

		if (iic_control(RTC_Read, buff, 1))
			return;

		printf(" clock base ");
		switch (buff[0] & 0x30) {
		case 0x00:
			printf("32.768KHz");
			break;
		case 0x10:
			printf("50Hz");
			break;
		case 0x20:
			printf("event");
			break;
		case 0x30:
			printf("test mode");
			break;
		}

		if (buff[0] & 0x80)
			printf(" stopped");
		if (buff[0] & 0x04)
			printf(" alarm enabled");
		sc->sc_flags &= ~RTC_BROKEN;
	}

	printf("\n");

	ta.ta_name = "todclock";
	ta.ta_rtc_arg = NULL;
	ta.ta_rtc_write = rtc_write; 
	ta.ta_rtc_read =  rtc_read;
	ta.ta_flags = 0;
	config_found(self, &ta, NULL);
}


int
rtcopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct rtc_softc *sc;
	int unit = minor(dev);
    
	if (unit >= rtc_cd.cd_ndevs)
		return(ENXIO);

	sc = rtc_cd.cd_devs[unit];
    
	if (!sc) return(ENXIO);

	if (sc->sc_flags & RTC_BROKEN) return(ENXIO);
	if (sc->sc_flags & RTC_OPEN) return(EBUSY);

	sc->sc_flags |= RTC_OPEN;

	return(0);
}


int
rtcclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = minor(dev);
	struct rtc_softc *sc = rtc_cd.cd_devs[unit];
    
	sc->sc_flags &= ~RTC_OPEN;

	return(0);
}


int
rtcread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	rtc_t rtc;
	int s;
	char buffer[32];
	int length;

	s = splclock();
	if (rtc_read(NULL, &rtc) == 0) {
		(void)splx(s);
		return(ENXIO);
	}

	(void)splx(s);

	sprintf(buffer, "%02d:%02d:%02d.%02d%02d %02d/%02d/%02d%02d\n",
	    rtc.rtc_hour, rtc.rtc_min, rtc.rtc_sec, rtc.rtc_centi,
	    rtc.rtc_micro, rtc.rtc_day, rtc.rtc_mon, rtc.rtc_cen,
	    rtc.rtc_year);

	if (uio->uio_offset > strlen(buffer))
		return 0;

	length = strlen(buffer) - uio->uio_offset;
	if (length > uio->uio_resid)
		length = uio->uio_resid;

	return(uiomove((caddr_t)buffer, length, uio));
}


static int
twodigits(buffer, pos)
	char *buffer;
	int pos;
{
	int result = 0;

	if (buffer[pos] >= '0' && buffer[pos] <= '9')
		result = (buffer[pos] - '0') * 10;
	if (buffer[pos+1] >= '0' && buffer[pos+1] <= '9')
		result += (buffer[pos+1] - '0');
	return(result);
}

int
rtcwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	rtc_t rtc;
	int s;
	char buffer[25];
	int length;
	int error;

	/*
	 * We require atomic updates!
	 */
	length = uio->uio_resid;
	if (uio->uio_offset || (length != sizeof(buffer)
	  && length != sizeof(buffer - 1)))
		return(EINVAL);
	
	if ((error = uiomove((caddr_t)buffer, sizeof(buffer), uio)))
		return(error);

	if (length == sizeof(buffer) && buffer[sizeof(buffer) - 1] != '\n')
		return(EINVAL);

	printf("rtcwrite: %s\n", buffer);

	rtc.rtc_micro = 0;
	rtc.rtc_centi = twodigits(buffer, 9);
	rtc.rtc_sec   = twodigits(buffer, 6);
	rtc.rtc_min   = twodigits(buffer, 3);
	rtc.rtc_hour  = twodigits(buffer, 0);
	rtc.rtc_day   = twodigits(buffer, 14);
	rtc.rtc_mon   = twodigits(buffer, 17);
	rtc.rtc_year  = twodigits(buffer, 22); 
	rtc.rtc_cen   = twodigits(buffer, 20); 

	s = splclock();
	rtc_write(NULL, &rtc);
	(void)splx(s);

	return(0);
}


int
rtcioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
/*	struct rtc_softc *sc = rtc_cd.cd_devs[minor(dev)];*/

/*	switch (cmd) {
	case RTCIOC_READ:
		return(0);
	}*/

	return(EINVAL);
}

/* End of rtc.c */
