#	$NetBSD: Makefile,v 1.2.4.2 2014/05/22 11:41:14 yamt Exp $
#

LIB=	rumpkern_sys_cygwin

SRCS=	rump_cygwin_compat.c rump_cygwin_sysent.c
SRCS+=	sys_cygwin_component.c

# XXX
CPPFLAGS+= -I${.CURDIR} -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
