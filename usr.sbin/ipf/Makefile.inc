#	$NetBSD: Makefile.inc,v 1.7 2000/05/03 11:40:54 veego Exp $

CPPFLAGS+=-I${.CURDIR}/../../../sys -I${.CURDIR}/../../../sys/netinet
CPPFLAGS+=-DUSE_INET6

.PATH:	${.CURDIR}/../../../dist/ipf \
	${.CURDIR}/../../../dist/ipf/man

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
