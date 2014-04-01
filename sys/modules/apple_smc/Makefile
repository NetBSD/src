# $NetBSD: Makefile,v 1.2 2014/04/01 17:48:39 riastradh Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc
SRCS=	apple_smc.c

WARNS=	4

# XXX What's the right thing here?
CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
