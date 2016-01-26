#	$NetBSD: Makefile,v 1.7 2016/01/26 23:12:17 pooka Exp $
#

LIB=	rumpkern_sys_sunos
COMMENT=SunOS/Solarisa system call translation

SRCS=	rump_sunos_compat.c
SRCS+=	sys_sunos_component.c

SYS_P=rump_sunos_sys
.include "../Makefile.sys"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
