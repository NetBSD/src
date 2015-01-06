#	$NetBSD: Makefile,v 1.7 2015/01/06 03:41:42 christos Exp $

.include <bsd.own.mk>

TESTSDIR=       ${TESTSBASE}/net/in_cksum

TESTS_SH=t_in_cksum

CDIR=${NETBSDSRCDIR}/sys/netinet

.for i in ${MACHINE_CPU} ${MACHINE_ARCH} ${MACHINE}
ADIR:=${NETBSDSRCDIR}/sys/arch/${i}/${i}
.if exists(${ADIR}/cpu_in_cksum.S)
.PATH.S: ${ADIR}
SRCS.in_cksum=cpu_in_cksum.S in_cksum.c
cpu_in_cksum.S: assym.h
CPPFLAGS+=-DHAVE_CPU_IN_CKSUM -I${.CURDIR}
.endif
.endfor

PROGS=in_cksum
MAN.in_cksum=    # empty
BINDIR.in_cksum= ${TESTSDIR}

CPPFLAGS+=-I${CDIR}

.include <bsd.test.mk>
