#	$NetBSD: Makefile,v 1.2 2001/01/07 09:03:21 martin Exp $

PROG	=	isdnd
SRCS	=	rc_parse.y rc_scan.l main.c rc_config.c log.c curses.c	\
		process.c rates.c msghdl.c fsm.c support.c timer.c	\
		exec.c dial.c monitor.c pcause.c controller.c alias.c	\
		holiday.c

COPTS	+=	-I${.CURDIR}/../isdnmonitor -I${.CURDIR}/../isdntel -I${.OBJDIR}

# compile debug support
COPTS	+=	-DDEBUG

MAN      =	isdnd.8 isdnd.rc.5 isdnd.rates.5 isdnd.acct.5

LDADD+=-ly -ll
YFLAGS += -d
CLEANFILES+=	y.tab.h

.include <bsd.prog.mk>

.if !defined(I4B_WITHOUT_CURSES)
COPTS	+=	-DUSE_CURSES
LDADD   +=      -lcurses
LDADD   += 	-ltermlib
.endif

.if defined(I4B_EXTERNAL_MONITOR)
COPTS	+=	-DI4B_EXTERNAL_MONITOR
.if defined(I4B_NOTCPIP_MONITOR)
COPTS	+=	-DI4B_NOTCPIP_MONITOR
.endif
.endif
