#	$NetBSD: Makefile,v 1.7 1997/04/17 07:41:05 thorpej Exp $

R=	..
.PATH:	${.CURDIR}/${R}
PROG=	bootxx
SRCS=	srt0.S bootxx.c promdev.c dvma.c
KOBJS=	__main.o bzero.o urem.o udiv.o
NOMAN=	1
STRIPFLAG=
BINDIR=	/usr/mdec
CFLAGS= -DBOOTXX

LIBS!=	cd ${.CURDIR}/${R}; ${MAKE} sadep
KOBJDIR!= cd ${.CURDIR}/${R}; ${MAKE} kernlibdir
_KOBJS=${KOBJS:S,^,${KOBJDIR}/,g}

${PROG}:${OBJS} ${_KOBJS} ${LIBS}
	${LD} -o ${.TARGET} -N -T ${RELOC} -e start ${OBJS} ${_KOBJS} ${LIBS}
	@size ${.TARGET}

.include <bsd.prog.mk>
