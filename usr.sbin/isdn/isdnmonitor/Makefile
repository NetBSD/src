#	$NetBSD: Makefile,v 1.7 2012/08/10 12:10:28 joerg Exp $

PROG	=	isdnmonitor
SRCS	=	main.c curses.c
MAN8	=	isdnmonitor.8

CPPFLAGS+=	-DDEBUG	# compile debug support
LDADD   +=      -lcurses -lterminfo
MAN	=	isdnmonitor.8

.include <bsd.prog.mk>

COPTS.main.c+=		-Wno-pointer-sign
