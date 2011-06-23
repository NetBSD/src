#	$NetBSD: Makefile,v 1.5.4.1 2011/06/23 14:20:47 cherry Exp $

PROG	=	isdnmonitor
SRCS	=	main.c curses.c
MAN8	=	isdnmonitor.8

CPPFLAGS+=	-DDEBUG	# compile debug support
LDADD   +=      -lcurses -lterminfo
MAN	=	isdnmonitor.8

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.main.c+=		-Wno-pointer-sign
.endif
