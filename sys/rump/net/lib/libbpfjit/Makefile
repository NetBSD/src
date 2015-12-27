#	$NetBSD: Makefile,v 1.1.12.1 2015/12/27 12:10:17 skrll Exp $
#
# Public Domain.
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_bpfjit
COMMENT=JIT compiler for Berkeley Packet Filter

SRCS=	bpfjit.c                                                                

CPPFLAGS+=	-I ${NETBSDSRCDIR}/sys/external/bsd/sljit/dist/sljit_src    

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
