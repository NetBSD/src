#	$NetBSD: Makefile,v 1.2 2018/11/26 08:48:43 mlelstv Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_nfssrv

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DNFSSERVER
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.PATH:	${S}/compat/netbsd32
SRCS+=	netbsd32_nfssvc.c

.include <bsd.kmodule.mk>
