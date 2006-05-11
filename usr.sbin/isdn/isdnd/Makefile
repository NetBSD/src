#	$NetBSD: Makefile,v 1.6 2006/05/11 08:55:10 mrg Exp $

PROG	=	isdnd
SRCS	=	rc_parse.y rc_scan.l main.c rc_config.c log.c curses.c	\
		process.c rates.c msghdl.c fsm.c support.c timer.c	\
		exec.c dial.c monitor.c pcause.c controller.c alias.c	\
		holiday.c

CPPFLAGS+=	-I${.CURDIR}/../isdnmonitor -I${.CURDIR}/../isdntel
CPPFLAGS+=	-I${.CURDIR} -I${.OBJDIR}

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

.if ${HAVE_GCC} == 4
COPTS.alias.c+=		-Wno-pointer-sign
COPTS.holiday.c+=	-Wno-pointer-sign
COPTS.monitor.c+=	-Wno-pointer-sign
.endif
