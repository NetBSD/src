#	$NetBSD: Makefile.inc,v 1.5.2.1 1999/12/20 21:08:06 he Exp $

CPPFLAGS+=-I${.CURDIR}/../../../sys -I${.CURDIR}/../../../sys/netinet

.PATH:	${.CURDIR}/../../../dist/ipf \
	${.CURDIR}/../../../dist/ipf/man

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
