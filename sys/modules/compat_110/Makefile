#	$NetBSD: Makefile,v 1.2.2.1 2026/11/04 23:27:54 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_110

CPPFLAGS+=	-DCOMPAT_110

SRCS+=	compat_110_mod.c uvm_110.c

.include <bsd.kmodule.mk>
