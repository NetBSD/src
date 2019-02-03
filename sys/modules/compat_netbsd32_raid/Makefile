#	$NetBSD: Makefile,v 1.1 2019/02/03 08:02:25 pgoyette Exp $

.include "../Makefile.inc"

KMOD=	compat_netbsd32_raid

.PATH:	${S}/dev/raidframe

CPPFLAGS+=	-DCOMPAT_NETBSD32

SRCS+=	rf_compat32.c 

.include <bsd.kmodule.mk>
