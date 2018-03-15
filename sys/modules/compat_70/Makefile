#	$NetBSD: Makefile,v 1.1.2.1 2018/03/15 05:10:05 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat

CPPFLAGS+=	-DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_70_mod.c rtsock_70.c uipc_usrreq_70.c

.include <bsd.kmodule.mk>
