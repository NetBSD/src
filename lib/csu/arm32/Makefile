#	$NetBSD: Makefile,v 1.14 2008/07/21 15:34:58 lukem Exp $

.include <bsd.own.mk>

OBJS+=		c++rt0.o

c++rt0.o: c++rt0.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} -fpic ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

.include "${.CURDIR}/../common_aout/Makefile.inc"
