/*	$NetBSD: clock_machdep.c,v 1.3 2003/08/07 23:14:13 marcus Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock_machdep.c,v 1.3 2003/08/07 23:14:13 marcus Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/clock_subr.h>
#include <sh3/clock.h>

#include <dreamcast/dev/g2/g2busvar.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

#define DREAMCAST_RTC		0x00710000

STATIC void dreamcast_rtc_init(void *);
STATIC void dreamcast_rtc_get(void *, time_t, struct clock_ymdhms *);
STATIC void dreamcast_rtc_set(void *, struct clock_ymdhms *);

STATIC u_int32_t dreamcast_read_rtc(void);
STATIC void dreamcast_write_rtc(u_int32_t);

STATIC struct rtc_ops dreamcast_rtc_ops = {
	.init	= dreamcast_rtc_init,
	.get	= dreamcast_rtc_get,
	.set	= dreamcast_rtc_set
};

static bus_space_tag_t rtc_tag;
static bus_space_handle_t rtc_handle;

void
machine_clock_init()
{

	sh_clock_init(SH_CLOCK_NORTC, &dreamcast_rtc_ops); 	
}

void
dreamcast_rtc_init(void *cookie)
{
	struct device *dv;

	/* Find g2bus device */
	for (dv = TAILQ_FIRST(&alldevs); dv != NULL;
	     dv = TAILQ_NEXT(dv, dv_list))
		if (!strcmp(dv->dv_xname, "g2bus0"))
			break;

	if (!dv)
		panic("No g2bus!");

	rtc_tag = &((struct g2bus_softc *)dv)->sc_memt;

	bus_space_map(rtc_tag, DREAMCAST_RTC, 12, 0, &rtc_handle);
}

void
dreamcast_rtc_get(void *cookie, time_t base, struct clock_ymdhms *dt)
{

	clock_secs_to_ymdhms(dreamcast_read_rtc(), dt);
}

void
dreamcast_rtc_set(void *cookie, struct clock_ymdhms *dt)
{
	
	dreamcast_write_rtc(clock_ymdhms_to_secs(dt));
}

u_int32_t
dreamcast_read_rtc()
{
	u_int32_t new, old;
	int i;
	
	for (old = 0;;) {
		for (i = 0; i < 3; i++) {
			new = ((bus_space_read_4(rtc_tag, rtc_handle, 0)
				& 0xffff) << 16) 
			  | (bus_space_read_4(rtc_tag, rtc_handle, 4) 
			     & 0xffff);
			if (new != old)
				break;
		}
		if (i < 3)
			old = new;
		else
			break;
	}

	/* offset 20 years */
	return (new - 631152000);
}

void
dreamcast_write_rtc(u_int32_t secs)
{
	u_int32_t new;
	int i, retry;

	/* offset 20 years */
	secs += 631152000;

	for (retry = 0; retry < 5; retry++) {

		/* Don't change an order */
		bus_space_write_4(rtc_tag, rtc_handle, 8, 1);
		bus_space_write_4(rtc_tag, rtc_handle, 4, secs & 0xffff);
		bus_space_write_4(rtc_tag, rtc_handle, 0, secs >> 16);

		/* verify */
		for (i = 0; i < 3; i++) {
			new = ((bus_space_read_4(rtc_tag, rtc_handle, 0)
				& 0xffff) << 16)
			  | (bus_space_read_4(rtc_tag, rtc_handle, 4)
			     & 0xffff);
			if (new == secs)
				return;
		}
	}
	/* set failure. but nothing to do */
}

