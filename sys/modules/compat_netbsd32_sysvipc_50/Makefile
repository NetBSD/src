#	$NetBSD: Makefile,v 1.4 2019/09/26 02:01:04 christos Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_sysvipc_50

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_50
CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.PATH:	${S}/compat/netbsd32

SRCS+=	netbsd32_compat_50_sysv.c

.include <bsd.kmodule.mk>
