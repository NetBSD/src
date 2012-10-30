#	$NetBSD: Makefile,v 1.6.2.1 2012/10/30 19:00:36 yamt Exp $

PROG	=	isdnmonitor
SRCS	=	main.c curses.c
MAN8	=	isdnmonitor.8

CPPFLAGS+=	-DDEBUG	# compile debug support
LDADD   +=      -lcurses -lterminfo
MAN	=	isdnmonitor.8

.include <bsd.prog.mk>

COPTS.main.c+=		-Wno-pointer-sign
