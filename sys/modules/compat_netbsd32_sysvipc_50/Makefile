#	$NetBSD: Makefile,v 1.3 2019/02/20 05:11:32 rin Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_sysvipc_50

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_50
CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.PATH:	${S}/compat/netbsd32

SRCS+=	netbsd32_compat_50_sysv.c

.if !empty(MACHINE_ARCH:Mmips64*)
WARNS=	3
.endif

.include <bsd.kmodule.mk>
