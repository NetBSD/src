#	$NetBSD: Makefile,v 1.3 2006/05/11 08:55:10 mrg Exp $

PROG	=	isdnmonitor
SRCS	=	main.c curses.c
MAN8	=	isdnmonitor.8

CPPFLAGS+=	-DDEBUG	# compile debug support
LDADD   +=      -lcurses
MAN	=	isdnmonitor.8

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.main.c+=		-Wno-pointer-sign
.endif
