#	$NetBSD: Makefile,v 1.3 2026/03/23 23:44:12 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_110

CPPFLAGS+=	-DCOMPAT_110

SRCS+=	compat_110_mod.c uvm_110.c

.include <bsd.kmodule.mk>
