#	$NetBSD: Makefile,v 1.1 2020/11/01 18:51:03 pgoyette Exp $

.include "../Makefile.inc"

KMOD=	compat_netbsd32_coredump

.PATH:	${S}/compat/netbsd32
SRCS=	netbsd32_core.c

.PATH:	${S}/kern
SRCS+=	core_elf32.c

CPPFLAGS+=	-DCOMPAT_NETBSD32

WARNS=	3

.include <bsd.kmodule.mk>
