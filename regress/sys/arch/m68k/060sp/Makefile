#	$NetBSD: Makefile,v 1.12 2003/10/26 07:25:35 lukem Exp $

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
	${HOST_SH} ${SPDIR}/asm2gas ${.IMPSRC} >${.TARGET}

.PATH: ${SPDIR}/dist

realdepend dotestsubs.o: itest.S ftest.S

regress: ${PROG}
	@/sbin/sysctl hw.model | grep -v -q 68060 || \
		./${PROG} | tee /dev/tty | cmp -s - ${.CURDIR}/goodoutput


.include <bsd.prog.mk>
