#	$NetBSD: Makefile,v 1.2.20.1 2008/09/18 04:30:05 wrstuden Exp $

PROG	 =	isdntel
SRCS	 =	main.c display.c files.c alias.c
LDADD	+=	-lcurses -ltermlib
MAN      =	isdntel.8

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.alias.c+=		-Wno-pointer-sign
.endif
