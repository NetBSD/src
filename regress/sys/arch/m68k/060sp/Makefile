#	$NetBSD: Makefile,v 1.4 1999/02/13 02:54:33 lukem Exp $

MKMAN=no
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
	@/usr/sbin/sysctl hw.model | grep -v -q 68060 || \
		./${PROG} | tee /dev/tty | cmp -s - ${.CURDIR}/goodoutput


.include <bsd.prog.mk>
