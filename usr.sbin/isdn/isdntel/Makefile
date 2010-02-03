#	$NetBSD: Makefile,v 1.4 2010/02/03 15:34:46 roy Exp $

PROG	 =	isdntel
SRCS	 =	main.c display.c files.c alias.c
LDADD	+=	-lcurses -lterminfo
MAN      =	isdntel.8

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.alias.c+=		-Wno-pointer-sign
.endif
