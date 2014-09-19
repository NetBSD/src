#	$NetBSD: Makefile,v 1.15 2014/09/19 21:35:42 joerg Exp $

NOMAN=		# defined

.include <bsd.own.mk>

PROG=		dotest
SRCS=		dotest.c dotestsubs.S
AFLAGS+=	-I.
CPPFLAGS+=	-I.
SPDIR=		${NETBSDSRCDIR}/sys/arch/m68k/060sp
DPSRCS+=	itest.S ftest.S
CLEANFILES+=	itest.S ftest.S

.SUFFIXES: .S .sa

.sa.S:
	SED=${TOOL_SED:Q} \
	${HOST_SH} ${SPDIR}/asm2gas ${.IMPSRC} >${.TARGET}

.PATH: ${SPDIR}/dist

dotestsubs.o: itest.S ftest.S

regress: ${PROG}
	@/sbin/sysctl hw.model | grep -v -q 68060 || \
		./${PROG} | tee /dev/tty | cmp -s - ${.CURDIR}/goodoutput


.include <bsd.prog.mk>
