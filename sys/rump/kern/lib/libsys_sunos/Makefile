#	$NetBSD: Makefile,v 1.1 2013/04/09 13:08:33 pooka Exp $
#

LIB=	rumpkern_sys_sunos

SRCS=	rump_sunos_compat.c rump_sunos_sysent.c component.c

# XXX
CPPFLAGS+= -I${.CURDIR} -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
