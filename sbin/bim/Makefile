#	$NetBSD: Makefile,v 1.7 2002/12/12 11:06:12 scw Exp $

.if (${MACHINE} == "pc532")
PROG   = bim
SRCS   = bim.c command.c
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

MAN  = bim.8
MANSUBDIR = /pc532

.include <bsd.prog.mk>

