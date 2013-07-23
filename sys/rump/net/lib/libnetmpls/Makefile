# $NetBSD: Makefile,v 1.1.2.2 2013/07/23 21:07:37 riastradh Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netmpls

LIB=	rumpnet_netmpls

SRCS=	mpls_ttl.c if_mpls.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../libnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
