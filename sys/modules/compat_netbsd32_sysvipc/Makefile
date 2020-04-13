#	$NetBSD: Makefile,v 1.1.22.2 2020/04/13 08:05:10 martin Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_sysvipc

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_10 -DCOMPAT_13 -DCOMPAT_14 -DCOMPAT_50
CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.PATH:	${S}/compat/netbsd32

SRCS+=	netbsd32_ipc.c

.include <bsd.kmodule.mk>
