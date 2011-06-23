#	$NetBSD: Makefile,v 1.4.4.1 2011/06/23 14:20:47 cherry Exp $

PROG	 =	isdntel
SRCS	 =	main.c display.c files.c alias.c
LDADD	+=	-lcurses -lterminfo
MAN      =	isdntel.8

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.alias.c+=		-Wno-pointer-sign
.endif
