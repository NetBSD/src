#	$NetBSD: Makefile,v 1.2 2019/02/20 05:11:31 rin Exp $

.include "../Makefile.inc"

KMOD=	compat_netbsd32_raid

.PATH:	${S}/dev/raidframe

CPPFLAGS+=	-DCOMPAT_NETBSD32

SRCS+=	rf_compat32.c 

.if !empty(MACHINE_ARCH:Mmips64*)
WARNS=	3
.endif

.include <bsd.kmodule.mk>
