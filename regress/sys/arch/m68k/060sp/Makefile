#	$NetBSD: Makefile,v 1.11 2003/09/30 03:34:42 mycroft Exp $

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
	@/sbin/sysctl hw.model | grep -v -q 68060 || \
		./${PROG} | tee /dev/tty | cmp -s - ${.CURDIR}/goodoutput


.include <bsd.prog.mk>
