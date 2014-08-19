/*	$NetBSD: ntpdtest.h,v 1.1.1.1.8.2 2014/08/19 23:51:49 tls Exp $	*/

#include "tests_main.h"

extern "C" {
#include "ntp_stdlib.h"
#include "ntp_calendar.h"
};

class ntpdtest : public ntptest {

protected:
	static time_t timefunc(time_t*);
	static time_t nowtime;
	static void   settime(int y, int m, int d, int H, int M, int S);

};
