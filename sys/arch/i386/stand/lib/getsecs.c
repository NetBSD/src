/*	$NetBSD: getsecs.c,v 1.3 2008/12/14 17:03:43 christos Exp $	*/

/* extracted from netbsd:sys/arch/i386/netboot/misc.c */

#include <sys/types.h>

#include <lib/libsa/stand.h>

#include "libi386.h"

extern int biosgetrtc(u_long*);

static inline u_long bcd2dec(u_long);

static inline u_long
bcd2dec(u_long arg)
{
	return (arg >> 4) * 10 + (arg & 0x0f);
}

time_t
getsecs(void) {
	/*
	 * Return the current time in seconds
	 */

	u_long t;
	time_t sec;

	if (biosgetrtc(&t))
		panic("RTC invalid");

	sec = bcd2dec(t & 0xff);
	sec *= 60;
	t >>= 8;
	sec += bcd2dec(t & 0xff);
	sec *= 60;
	t >>= 8;
	sec += bcd2dec(t & 0xff);

	return sec;
}
