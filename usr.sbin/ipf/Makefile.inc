#	$NetBSD: Makefile.inc,v 1.7.4.1 2001/07/29 19:29:53 he Exp $

CPPFLAGS+=-I${.CURDIR}/../../../sys -I${.CURDIR}/../../../sys/netinet
CPPFLAGS+=-DUSE_INET6 -DSTATETOP

.PATH:	${.CURDIR}/../../../dist/ipf \
	${.CURDIR}/../../../dist/ipf/man

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
