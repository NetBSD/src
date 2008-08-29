#	$NetBSD: Makefile,v 1.4 2008/08/29 00:02:25 gmcgarry Exp $

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
