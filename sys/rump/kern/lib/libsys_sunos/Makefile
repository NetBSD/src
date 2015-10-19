#	$NetBSD: Makefile,v 1.6 2015/10/19 16:16:37 pooka Exp $
#

LIB=	rumpkern_sys_sunos
COMMENT=SunOS/Solarisa system call translation

SRCS=	rump_sunos_compat.c
SRCS+=	sys_sunos_component.c

# XXX
CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

SYS_P=rump_sunos_sys
.include "../Makefile.sys"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
