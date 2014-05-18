#	$NetBSD: Makefile,v 1.1.6.1 2014/05/18 17:46:17 rmind Exp $
#

LIB=	rumpkern_sys_cygwin

SRCS=	rump_cygwin_compat.c rump_cygwin_sysent.c
SRCS+=	sys_cygwin_component.c

# XXX
CPPFLAGS+= -I${.CURDIR} -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
