#	$NetBSD: Makefile,v 1.9 2001/12/12 01:24:13 tv Exp $

PROG=		dotest
SRCS=		dotest.c dotestsubs.S
NOMAN=		# defined
AFLAGS+=	-I.
CPPFLAGS+=	-I.
SPDIR=		${.CURDIR}/../../../../../sys/arch/m68k/060sp
CLEANFILES+=	itest.S ftest.S

.SUFFIXES: .S .sa

.sa.S:
	sh ${SPDIR}/asm2gas ${.IMPSRC} >${.TARGET}

.PATH: ${SPDIR}/dist

realdepend dotestsubs.o: itest.S ftest.S

regress: ${PROG}
	@/usr/sbin/sysctl hw.model | grep -v -q 68060 || \
		./${PROG} | tee /dev/tty | cmp -s - ${.CURDIR}/goodoutput


.include <bsd.prog.mk>
