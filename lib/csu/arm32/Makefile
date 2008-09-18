#	$NetBSD: Makefile,v 1.13.44.1 2008/09/18 04:39:18 wrstuden Exp $

.include <bsd.own.mk>

OBJS+=		c++rt0.o

c++rt0.o: c++rt0.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} -fpic ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

.include "${.CURDIR}/../common_aout/Makefile.inc"
