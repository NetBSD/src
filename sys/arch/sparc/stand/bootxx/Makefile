#	$NetBSD: Makefile,v 1.2 1995/09/18 23:32:41 pk Exp $

R=	..
.PATH:	${R}
PROG=	bootxx
SRCS=	srt0.S bootxx.c promdev.c dvma.c
NOMAN=	1
STRIP=
BINDIR=	/usr/mdec
LIBS!=	cd ./${R}; ${MAKE} libdep

CFLAGS= -DBOOTXX

bootxx:	${OBJS} ${LIBS:S,^,../,g}
	${LD} -N -T ${RELOC} -e start ${OBJS} ${LIBS:S,^,../,g}
	# convert to Sun magic
	(echo -n 01 | tr 01 '\01\03'; tail +3c a.out) > ${.TARGET}
	@rm a.out
	@size ${.TARGET}

.include <bsd.prog.mk>
