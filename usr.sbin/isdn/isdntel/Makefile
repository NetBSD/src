#	$NetBSD: Makefile,v 1.2 2006/05/11 08:55:10 mrg Exp $

PROG	 =	isdntel
SRCS	 =	main.c display.c files.c alias.c
LDADD	+=	-lcurses -ltermlib
MAN      =	isdntel.8

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.alias.c+=		-Wno-pointer-sign
.endif
