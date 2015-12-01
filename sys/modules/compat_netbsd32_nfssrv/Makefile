#	$NetBSD: Makefile,v 1.1 2015/12/01 09:13:58 pgoyette Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_nfssrv

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DNFSSERVER

.PATH:	${S}/compat/netbsd32
SRCS+=	netbsd32_nfssvc.c

.include <bsd.kmodule.mk>
