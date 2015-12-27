#	$NetBSD: Makefile,v 1.3.8.3 2015/12/27 12:10:16 skrll Exp $
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
