#	$NetBSD: Makefile,v 1.1.4.4 2017/12/03 11:39:15 jdolecek Exp $
#

LIB=	rumpkern_sys_sunos
COMMENT=SunOS/Solarisa system call translation

SRCS=	rump_sunos_compat.c
SRCS+=	sys_sunos_component.c

SYS_P=rump_sunos_sys
.include "../Makefile.sys"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
