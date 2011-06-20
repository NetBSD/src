#	$NetBSD: Makefile,v 1.6 2011/06/20 07:44:02 mrg Exp $

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
