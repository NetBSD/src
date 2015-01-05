#	$NetBSD: Makefile,v 1.1 2015/01/05 22:38:36 christos Exp $

.include <bsd.own.mk>

MKMAN=no
TESTSDIR=       ${TESTSBASE}/net/cksum

TESTS_SH=t_in_cksum

ADIR=${NETBSDSRCDIR}/sys/arch/${MACHINE}/${MACHINE}
CDIR=${NETBSDSRCDIR}/sys/netinet
.if exists(${ADIR}/cpu_in_cksum.S)
.PATH.S: ${ADIR}
SRCS+=cpu_in_cksum.S
CPPFLAGS+=-DHAVE_CPU_IN_CKSUM
.endif

PROGS=in_cksum

CPPFLAGS+=-I${CDIR}

.include <bsd.test.mk>

foo:
	echo ${ADIR}/cpu_in_cksum.S
