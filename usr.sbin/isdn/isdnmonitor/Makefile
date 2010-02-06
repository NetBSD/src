#	$NetBSD: Makefile,v 1.5 2010/02/06 23:45:27 he Exp $

PROG	=	isdnmonitor
SRCS	=	main.c curses.c
MAN8	=	isdnmonitor.8

CPPFLAGS+=	-DDEBUG	# compile debug support
LDADD   +=      -lcurses -lterminfo
MAN	=	isdnmonitor.8

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.main.c+=		-Wno-pointer-sign
.endif
