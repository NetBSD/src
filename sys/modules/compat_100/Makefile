#	$NetBSD: Makefile,v 1.3 2024/05/29 06:47:23 mlelstv Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_100

CPPFLAGS+=	-DCOMPAT_100

SRCS+=	compat_100_mod.c kern_event_100.c sys_descrip_100.c

.include <bsd.kmodule.mk>
