#	$NetBSD: Makefile,v 1.12 1999/02/27 03:28:30 tv Exp $

OBJS+=		c++rt0.o

c++rt0.o: c++rt0.c
	${COMPILE.c} -fpic ${.ALLSRC}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

.include "${.CURDIR}/../Makefile.csu.aout"
