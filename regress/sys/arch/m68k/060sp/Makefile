#	$NetBSD: Makefile,v 1.14 2008/03/28 22:29:16 apb Exp $

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
	SED=${TOOL_SED:Q} \
	${HOST_SH} ${SPDIR}/asm2gas ${.IMPSRC} >${.TARGET}

.PATH: ${SPDIR}/dist

realdepend dotestsubs.o: itest.S ftest.S

regress: ${PROG}
	@/sbin/sysctl hw.model | grep -v -q 68060 || \
		./${PROG} | tee /dev/tty | cmp -s - ${.CURDIR}/goodoutput


.include <bsd.prog.mk>
