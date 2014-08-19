# $NetBSD: Makefile,v 1.2.10.2 2014/08/20 00:04:31 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc
SRCS=	apple_smc.c

WARNS=	4

# XXX What's the right thing here?
CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
