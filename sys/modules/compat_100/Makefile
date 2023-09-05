#	$NetBSD: Makefile,v 1.2 2023/09/05 14:49:46 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_100

CPPFLAGS+=	-DCOMPAT_100

SRCS+=	compat_100_mod.c kern_event_100.c

.include <bsd.kmodule.mk>
