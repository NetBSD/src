#	$NetBSD: Makefile,v 1.10 2002/09/18 04:16:01 lukem Exp $

NOMAN=		# defined

.include <bsd.own.mk>

PROG=		dotest
SRCS=		dotest.c dotestsubs.S
AFLAGS+=	-I.
CPPFLAGS+=	-I.
SPDIR=		${NETBSDSRCDIR}/sys/arch/m68k/060sp
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
