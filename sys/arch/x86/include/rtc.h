/*      $NetBSD: rtc.h,v 1.1.2.2 2009/06/19 21:22:10 snj Exp $    */

#include <dev/clock_subr.h>

void rtc_register(void);
int  rtc_get_ymdhms(todr_chip_handle_t, struct clock_ymdhms *);
int  rtc_set_ymdhms(todr_chip_handle_t, struct clock_ymdhms *);
