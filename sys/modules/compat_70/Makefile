#	$NetBSD: Makefile,v 1.1.2.2 2018/03/15 07:29:02 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_70

CPPFLAGS+=	-DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_70_mod.c rtsock_70.c uipc_usrreq_70.c

.include <bsd.kmodule.mk>
