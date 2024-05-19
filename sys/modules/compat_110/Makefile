#	$NetBSD: Makefile,v 1.1 2024/05/19 22:25:49 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_110

CPPFLAGS+=	-DCOMPAT_110

SRCS+=	compat_110_mod.c sys_decrip_110.c

.include <bsd.kmodule.mk>
