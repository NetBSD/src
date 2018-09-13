#	$NetBSD: Makefile,v 1.1.2.1 2018/09/13 03:52:35 pgoyette Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_sysvipc_50

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_50
CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG

.PATH:	${S}/compat/netbsd32

SRCS+=	netbsd32_compat_50_sysv.c

.include <bsd.kmodule.mk>
