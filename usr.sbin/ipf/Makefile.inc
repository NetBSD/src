#	$NetBSD: Makefile.inc,v 1.6 1999/12/12 11:29:55 veego Exp $

CPPFLAGS+=-I${.CURDIR}/../../../sys -I${.CURDIR}/../../../sys/netinet

.PATH:	${.CURDIR}/../../../dist/ipf \
	${.CURDIR}/../../../dist/ipf/man

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
