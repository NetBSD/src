#	$NetBSD: Makefile,v 1.3.8.4 2016/03/19 11:30:36 skrll Exp $
#

LIB=	rumpkern_sys_sunos
COMMENT=SunOS/Solarisa system call translation

SRCS=	rump_sunos_compat.c
SRCS+=	sys_sunos_component.c

SYS_P=rump_sunos_sys
.include "../Makefile.sys"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
