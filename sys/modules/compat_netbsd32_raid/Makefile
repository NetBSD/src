#	$NetBSD: Makefile,v 1.3 2019/09/26 02:01:04 christos Exp $

.include "../Makefile.inc"

KMOD=	compat_netbsd32_raid

.PATH:	${S}/dev/raidframe

CPPFLAGS+=	-DCOMPAT_NETBSD32

SRCS+=	rf_compat32.c 

.include <bsd.kmodule.mk>
