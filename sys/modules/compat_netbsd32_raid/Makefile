#	$NetBSD: Makefile,v 1.2.4.3 2020/04/13 08:05:09 martin Exp $

.include "../Makefile.inc"

KMOD=	compat_netbsd32_raid

.PATH:	${S}/dev/raidframe

CPPFLAGS+=	-DCOMPAT_NETBSD32

SRCS+=	rf_compat32.c 

.include <bsd.kmodule.mk>
