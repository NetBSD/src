#	$NetBSD: Makefile,v 1.2.4.2 2019/06/10 22:09:18 christos Exp $

.include "../Makefile.inc"

KMOD=	compat_netbsd32_raid

.PATH:	${S}/dev/raidframe

CPPFLAGS+=	-DCOMPAT_NETBSD32

SRCS+=	rf_compat32.c 

.if !empty(MACHINE_ARCH:Mmips64*)
WARNS=	3
.endif

.include <bsd.kmodule.mk>
