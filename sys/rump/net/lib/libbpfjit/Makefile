#	$NetBSD: Makefile,v 1.1.10.3 2017/12/03 11:39:17 jdolecek Exp $
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
