#	$NetBSD: Makefile,v 1.2 2019/01/27 02:08:45 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_70

CPPFLAGS+=	-DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_70_mod.c rtsock_70.c uipc_usrreq_70.c

.include <bsd.kmodule.mk>
