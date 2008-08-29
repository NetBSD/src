#	$NetBSD: Makefile,v 1.3 2008/08/29 00:02:25 gmcgarry Exp $

PROG	 =	isdntel
SRCS	 =	main.c display.c files.c alias.c
LDADD	+=	-lcurses -ltermlib
MAN      =	isdntel.8

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.alias.c+=		-Wno-pointer-sign
.endif
