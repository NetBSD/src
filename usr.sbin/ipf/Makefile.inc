#	$NetBSD: Makefile.inc,v 1.8 2001/03/13 16:29:42 christos Exp $

CPPFLAGS+=-I${.CURDIR}/../../../sys -I${.CURDIR}/../../../sys/netinet
CPPFLAGS+=-DUSE_INET6 -DSTATETOP

.PATH:	${.CURDIR}/../../../dist/ipf \
	${.CURDIR}/../../../dist/ipf/man

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
