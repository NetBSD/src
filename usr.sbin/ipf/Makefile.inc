#	$NetBSD: Makefile.inc,v 1.5.6.1 1999/12/27 18:37:42 wrstuden Exp $

CPPFLAGS+=-I${.CURDIR}/../../../sys -I${.CURDIR}/../../../sys/netinet

.PATH:	${.CURDIR}/../../../dist/ipf \
	${.CURDIR}/../../../dist/ipf/man

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
