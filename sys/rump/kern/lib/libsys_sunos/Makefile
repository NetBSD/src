#	$NetBSD: Makefile,v 1.1.4.2 2013/06/23 06:20:28 tls Exp $
#

LIB=	rumpkern_sys_sunos

SRCS=	rump_sunos_compat.c rump_sunos_sysent.c component.c

# XXX
CPPFLAGS+= -I${.CURDIR} -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
