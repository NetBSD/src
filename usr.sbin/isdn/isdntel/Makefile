#	$NetBSD: Makefile,v 1.7 2012/08/10 12:10:28 joerg Exp $

PROG	 =	isdntel
SRCS	 =	main.c display.c files.c alias.c
LDADD	+=	-lcurses -lterminfo
MAN      =	isdntel.8

.include <bsd.prog.mk>

COPTS.alias.c+=		-Wno-pointer-sign
COPTS.files.c+=		-Wno-format-nonliteral
