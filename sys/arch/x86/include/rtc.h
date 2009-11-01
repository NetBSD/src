/*      $NetBSD: rtc.h,v 1.1.6.2 2009/11/01 13:58:16 jym Exp $    */

#include <dev/clock_subr.h>

void rtc_register(void);
int  rtc_get_ymdhms(todr_chip_handle_t, struct clock_ymdhms *);
int  rtc_set_ymdhms(todr_chip_handle_t, struct clock_ymdhms *);
