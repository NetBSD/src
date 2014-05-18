#	$NetBSD: Makefile,v 1.1.4.2 2014/05/18 17:46:19 rmind Exp $
#
# Public Domain.
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_bpfjit

SRCS=	bpfjit.c                                                                

CPPFLAGS+=	-I ${NETBSDSRCDIR}/sys/external/bsd/sljit/dist/sljit_src    

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
