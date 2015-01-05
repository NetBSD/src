#	$NetBSD: Makefile,v 1.4 2015/01/05 23:48:46 christos Exp $

.include <bsd.own.mk>

MKMAN=no
TESTSDIR=       ${TESTSBASE}/net/cksum

TESTS_SH=t_in_cksum


CDIR=${NETBSDSRCDIR}/sys/netinet

.for i in ${MACHINE_CPU} ${MACHINE_ARCH} ${MACHINE}
ADIR:=${NETBSDSRCDIR}/sys/arch/${i}/${i}
.if exists(${ADIR}/cpu_in_cksum.S)
.PATH.S: ${ADIR}
SRCS.in_cksum=cpu_in_cksum.S in_cksum.c
cpu_in_cksum.S: assym.h
CPPFLAGS+=-DHAVE_CPU_IN_CKSUM -I.
.endif
.endfor

PROGS=in_cksum

CPPFLAGS+=-I${CDIR}

.include <bsd.test.mk>
