#	$NetBSD: Makefile,v 1.4 1996/11/29 15:00:25 pk Exp $

R=	..
.PATH:	${.CURDIR}/${R}
PROG=	boot
SRCS=	srt0.S boot.c promdev.c dvma.c net.c netif_sun.c conf.c version.c
NOMAN=	1
BINDIR=	/usr/mdec
LIBS!=	cd ${.CURDIR}/${R}; ${MAKE} libdep

${PROG}: ${OBJS} ${LIBS}
	${LD} -o ${.TARGET} -N -T ${RELOC} -e start ${OBJS} ${LIBS}
	@size ${.TARGET}

.include <bsd.prog.mk>
