#	$NetBSD: Makefile,v 1.1.10.2 2014/08/20 00:04:42 tls Exp $
#
# Public Domain.
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_bpfjit

SRCS=	bpfjit.c                                                                

CPPFLAGS+=	-I ${NETBSDSRCDIR}/sys/external/bsd/sljit/dist/sljit_src    

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
