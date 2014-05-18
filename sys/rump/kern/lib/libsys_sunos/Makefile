#	$NetBSD: Makefile,v 1.1.6.1 2014/05/18 17:46:18 rmind Exp $
#

LIB=	rumpkern_sys_sunos

SRCS=	rump_sunos_compat.c rump_sunos_sysent.c
SRCS+=	sys_sunos_component.c

# XXX
CPPFLAGS+= -I${.CURDIR} -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
