#	$NetBSD: Makefile,v 1.13 1999/03/19 22:54:40 thorpej Exp $

OBJS+=		c++rt0.o

c++rt0.o: c++rt0.c
	${COMPILE.c} -fpic ${.ALLSRC}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

.include "${.CURDIR}/../common_aout/Makefile.inc"
