#	$NetBSD: Makefile,v 1.3.8.2 2015/06/06 14:40:28 skrll Exp $
#

LIB=	rumpkern_sys_sunos

SRCS=	rump_sunos_compat.c
SRCS+=	sys_sunos_component.c

# XXX
CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

SYS_P=rump_sunos_sys
.include "../Makefile.sys"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
