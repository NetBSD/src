#	$NetBSD: Makefile,v 1.1.1.1 2001/01/06 13:00:27 martin Exp $

PROG	=	isdnmonitor
SRCS	=	main.c curses.c
MAN8	=	isdnmonitor.8

COPTS	+=	-DDEBUG	# compile debug support
LDADD   +=      -lcurses
MAN	=	isdnmonitor.8

.include <bsd.prog.mk>
