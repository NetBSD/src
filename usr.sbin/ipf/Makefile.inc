#	$NetBSD: Makefile.inc,v 1.10 2003/09/13 16:59:30 erh Exp $

.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/sys/netinet
CPPFLAGS+=-DUSE_INET6 -DSTATETOP -D__UIO_EXPOSE

.PATH:	${NETBSDSRCDIR}/dist/ipf \
	${NETBSDSRCDIR}/dist/ipf/man

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
