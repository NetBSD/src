#	$NetBSD: Makefile,v 1.2 2001/01/10 18:11:07 thorpej Exp $

PROG	=	isdnmonitor
SRCS	=	main.c curses.c
MAN8	=	isdnmonitor.8

CPPFLAGS+=	-DDEBUG	# compile debug support
LDADD   +=      -lcurses
MAN	=	isdnmonitor.8

.include <bsd.prog.mk>
