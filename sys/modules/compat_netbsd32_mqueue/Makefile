#	$NetBSD: Makefile,v 1.1.20.1 2018/11/26 12:25:20 pgoyette Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_mqueue

CPPFLAGS+=	-DCOMPAT_NETBSD32 -DCOMPAT_50
CPPFLAGS+=	-DMQUEUE
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.PATH:	${S}/compat/netbsd32
SRCS+=	netbsd32_mqueue.c

.include <bsd.kmodule.mk>
