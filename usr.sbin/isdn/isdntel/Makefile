#	$NetBSD: Makefile,v 1.5 2011/06/20 07:44:02 mrg Exp $

PROG	 =	isdntel
SRCS	 =	main.c display.c files.c alias.c
LDADD	+=	-lcurses -lterminfo
MAN      =	isdntel.8

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.alias.c+=		-Wno-pointer-sign
.endif
