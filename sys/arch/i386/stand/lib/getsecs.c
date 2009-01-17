/*	$NetBSD: getsecs.c,v 1.2.140.1 2009/01/17 13:28:05 mjf Exp $	*/

/* extracted from netbsd:sys/arch/i386/netboot/misc.c */

#include <sys/types.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>

#include "libi386.h"

static inline u_long bcd2dec(u_long);

static inline u_long
bcd2dec(u_long arg)
{
	return (arg >> 4) * 10 + (arg & 0x0f);
}

satime_t
getsecs(void) {
	/*
	 * Return the current time in seconds
	 */

	u_long t;
	satime_t sec;

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
