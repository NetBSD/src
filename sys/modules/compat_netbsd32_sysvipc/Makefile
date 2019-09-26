#	$NetBSD: Makefile,v 1.5 2019/09/26 02:01:04 christos Exp $

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
