#	$NetBSD: Makefile,v 1.1.4.3 2014/08/20 00:04:40 tls Exp $
#

LIB=	rumpkern_sys_cygwin

SRCS=	rump_cygwin_compat.c rump_cygwin_sysent.c
SRCS+=	sys_cygwin_component.c

# XXX
CPPFLAGS+= -I${.CURDIR} -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
