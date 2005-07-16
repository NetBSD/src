#	$NetBSD: Makefile.inc,v 1.11 2005/07/16 19:36:25 christos Exp $

.PATH: ${.CURDIR}/time

SRCS+=	_daylight.c							\
	asctime.c difftime.c localtime.c strftime.c strptime.c
MAN+=	ctime.3 offtime.3 time2posix.3 tzfile.5 tzset.3 strftime.3 strptime.3
CPPFLAGS+=-DALL_STATE -DUSG_COMPAT

MLINKS+=ctime.3 ctime_r.3	\
	ctime.3 asctime.3	\
	ctime.3 asctime_r.3	\
	ctime.3 difftime.3	\
	ctime.3 gmtime.3	\
	ctime.3 gmtime_r.3	\
	ctime.3 localtime.3	\
	ctime.3 localtime_r.3	\
	ctime.3 mktime.3	\
	offtime.3 timeoff.3	\
	offtime.3 timegm.3	\
	offtime.3 timelocal.3	\
	tzset.3 daylight.3	\
	tzset.3 tzsetwall.3
