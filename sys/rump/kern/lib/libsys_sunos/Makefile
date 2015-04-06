#	$NetBSD: Makefile,v 1.3.8.1 2015/04/06 15:18:30 skrll Exp $
#

LIB=	rumpkern_sys_sunos

SRCS=	rump_sunos_compat.c rump_sunos_sysent.c
SRCS+=	sys_sunos_component.c

# XXX
CPPFLAGS+= -I${.CURDIR} -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

SYS_P=rump_sunos_sys
.include "../Makefile.sys"
