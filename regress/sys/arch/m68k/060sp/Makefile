#	$NetBSD: Makefile,v 1.1.1.1 1997/09/30 23:12:45 is Exp $

NOMAN=notyet
PROG=dotest
SRCS=dotest.c dotestsubs.S
AFLAGS+=-I.
CFLAGS+=-I.
SPDIR=${.CURDIR}/../../../../../sys/arch/m68k/060sp
CLEANFILES+=itest.S ftest.S

.SUFFIXES: .S .sa

.sa.S: ; sh ${SPDIR}/asm2gas ${.IMPSRC} >${.TARGET}

.PATH: ${SPDIR}

depend dotestsubs.o: itest.S ftest.S

regress: ${PROG}
	@./${PROG} | tee /dev/tty | cmp -s - ${.CURDIR}/goodoutput


.include <bsd.prog.mk>
