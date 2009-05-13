#	$NetBSD: Makefile,v 1.7.6.1 2009/05/13 19:20:25 jym Exp $

PROG	=	isdnd
SRCS	=	rc_parse.y rc_scan.l main.c rc_config.c log.c curses.c	\
		process.c rates.c msghdl.c fsm.c support.c timer.c	\
		exec.c dial.c monitor.c pcause.c controller.c alias.c	\
		holiday.c

CPPFLAGS+=	-I${.CURDIR}/../isdnmonitor -I${.CURDIR}/../isdntel
CPPFLAGS+=	-I${.CURDIR} -I${.OBJDIR} -DYY_NO_INPUT

# compile debug support
CPPFLAGS+=	-DDEBUG

MAN      =	isdnd.8 isdnd.rc.5 isdnd.rates.5 isdnd.acct.5

LDADD+=-ly -ll
YHEADER=1

.include <bsd.prog.mk>

.if !defined(I4B_WITHOUT_CURSES)
CPPFLAGS+=	-DUSE_CURSES
LDADD   +=      -lcurses
LDADD   += 	-ltermlib
.endif

.if defined(I4B_EXTERNAL_MONITOR)
CPPFLAGS+=	-DI4B_EXTERNAL_MONITOR
.if defined(I4B_NOTCPIP_MONITOR)
CPPFLAGS+=	-DI4B_NOTCPIP_MONITOR
.endif
.endif

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.alias.c+=		-Wno-pointer-sign
COPTS.holiday.c+=	-Wno-pointer-sign
COPTS.monitor.c+=	-Wno-pointer-sign
.endif
