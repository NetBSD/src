#	$NetBSD: Makefile,v 1.1.6.2 2014/05/22 11:41:16 yamt Exp $
#
# Public Domain.
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_bpfjit

SRCS=	bpfjit.c                                                                

CPPFLAGS+=	-I ${NETBSDSRCDIR}/sys/external/bsd/sljit/dist/sljit_src    

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
