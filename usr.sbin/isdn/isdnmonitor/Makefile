#	$NetBSD: Makefile,v 1.3.20.1 2008/09/18 04:30:05 wrstuden Exp $

PROG	=	isdnmonitor
SRCS	=	main.c curses.c
MAN8	=	isdnmonitor.8

CPPFLAGS+=	-DDEBUG	# compile debug support
LDADD   +=      -lcurses
MAN	=	isdnmonitor.8

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.main.c+=		-Wno-pointer-sign
.endif
