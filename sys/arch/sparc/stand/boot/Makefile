#	$NetBSD: Makefile,v 1.1 1995/09/16 23:21:56 pk Exp $

R=	..
.PATH:	${R}
PROG=	boot
SRCS=	srt0.S boot.c promdev.c dvma.c net.c netif_sun.c conf.c version.c
NOMAN=	1
BINDIR=	/usr/mdec
LIBS!=	cd ./${R}; ${MAKE} libdep

${PROG}:${OBJS} ${LIBS:S,^,../,g}
	${LD} -N -T ${RELOC} -e start ${OBJS} ${LIBS:S,^,../,g}
	# convert to Sun magic
	(echo -n 01 | tr 01 '\01\03'; tail +3c a.out) > ${.TARGET}
	@rm a.out
	@size ${.TARGET}

.include <bsd.prog.mk>
