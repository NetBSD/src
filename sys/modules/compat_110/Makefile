#	$NetBSD: Makefile,v 1.2 2024/05/20 01:30:34 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_110

CPPFLAGS+=	-DCOMPAT_110

SRCS+=	compat_110_mod.c

.include <bsd.kmodule.mk>
