#	$NetBSD: Makefile,v 1.2 2018/11/26 08:48:43 mlelstv Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_sysvipc

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_10 -DCOMPAT_13 -DCOMPAT_14 -DCOMPAT_50
CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.PATH:	${S}/compat/netbsd32

SRCS+=	netbsd32_ipc.c
SRCS+=	netbsd32_compat_10.c
SRCS+=	netbsd32_compat_14.c
SRCS+=	netbsd32_compat_50_sysv.c

.include <bsd.kmodule.mk>
