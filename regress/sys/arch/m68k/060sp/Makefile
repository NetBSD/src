#	$NetBSD: Makefile,v 1.1.1.1.2.1 1997/11/08 07:39:45 lukem Exp $

NOMAN=notyet
PROG=dotest
SRCS=dotest.c dotestsubs.S
AFLAGS+=-I.
CPPFLAGS+=-I.
SPDIR=${.CURDIR}/../../../../../sys/arch/m68k/060sp
CLEANFILES+=itest.S ftest.S

.SUFFIXES: .S .sa

.sa.S: ; sh ${SPDIR}/asm2gas ${.IMPSRC} >${.TARGET}

.PATH: ${SPDIR}

depend dotestsubs.o: itest.S ftest.S

regress: ${PROG}
	@./${PROG} | tee /dev/tty | cmp -s - ${.CURDIR}/goodoutput


.include <bsd.prog.mk>
