#	$NetBSD: Makefile,v 1.1.20.1 2018/09/12 04:35:22 pgoyette Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_sysvipc

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_10 -DCOMPAT_13 -DCOMPAT_14 -DCOMPAT_50
CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG

.PATH:	${S}/compat/netbsd32

SRCS+=	netbsd32_ipc.c

.include <bsd.kmodule.mk>
