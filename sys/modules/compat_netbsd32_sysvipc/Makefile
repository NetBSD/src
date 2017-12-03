#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:51 jdolecek Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_sysvipc

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_10 -DCOMPAT_13 -DCOMPAT_14 -DCOMPAT_50
CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG

.PATH:	${S}/compat/netbsd32

SRCS+=	netbsd32_ipc.c
SRCS+=	netbsd32_compat_10.c
SRCS+=	netbsd32_compat_14.c
SRCS+=	netbsd32_compat_50_sysv.c

.include <bsd.kmodule.mk>
