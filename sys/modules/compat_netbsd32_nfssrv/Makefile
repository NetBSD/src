#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:51 jdolecek Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_nfssrv

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DNFSSERVER

.PATH:	${S}/compat/netbsd32
SRCS+=	netbsd32_nfssvc.c

.include <bsd.kmodule.mk>
