#	$NetBSD: Makefile.inc,v 1.9 2002/09/18 03:54:30 lukem Exp $

.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/sys/netinet
CPPFLAGS+=-DUSE_INET6 -DSTATETOP

.PATH:	${NETBSDSRCDIR}/dist/ipf \
	${NETBSDSRCDIR}/dist/ipf/man

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
