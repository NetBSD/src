#	$NetBSD: Makefile,v 1.6 2011/08/16 08:18:19 christos Exp $

PROG	 =	isdntel
SRCS	 =	main.c display.c files.c alias.c
LDADD	+=	-lcurses -lterminfo
MAN      =	isdntel.8

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.alias.c+=		-Wno-pointer-sign
.endif
COPTS.files.c+=		-Wno-format-nonliteral
