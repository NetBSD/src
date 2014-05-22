/*	$NetBSD: libntptest.h,v 1.1.1.1.4.2 2014/05/22 15:50:16 yamt Exp $	*/

#include "tests_main.h"

extern "C" {
#include "ntp_stdlib.h"
#include "ntp_calendar.h"
};

class libntptest : public ntptest {

protected:
	static time_t timefunc(time_t*);
	static time_t nowtime;
	static void   settime(int y, int m, int d, int H, int M, int S);

};
