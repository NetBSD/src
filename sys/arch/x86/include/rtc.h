/*      $NetBSD: rtc.h,v 1.1.12.2 2010/04/21 00:33:45 matt Exp $    */

#include <dev/clock_subr.h>

void rtc_register(void);
int  rtc_get_ymdhms(todr_chip_handle_t, struct clock_ymdhms *);
int  rtc_set_ymdhms(todr_chip_handle_t, struct clock_ymdhms *);
