#	$NetBSD: Makefile,v 1.3 1995/09/30 21:43:40 pk Exp $

R=	..
.PATH:	${.CURDIR}/${R}
PROG=	bootxx
SRCS=	srt0.S bootxx.c promdev.c dvma.c
NOMAN=	1
STRIP=
BINDIR=	/usr/mdec
LIBS!=	cd ${.CURDIR}/${R}; ${MAKE} libdep

CFLAGS= -DBOOTXX

${PROG}:${OBJS} ${LIBS}
	${LD} -N -T ${RELOC} -e start ${OBJS} ${LIBS}
	# convert to Sun magic
	(echo -n 01 | tr 01 '\01\03'; tail +3c a.out) > ${.TARGET}
	@rm a.out
	@size ${.TARGET}

.include <bsd.prog.mk>
