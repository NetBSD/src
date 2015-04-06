#	$NetBSD: Makefile,v 1.2.8.1 2015/04/06 15:18:29 skrll Exp $
#

LIB=	rumpkern_sys_cygwin

SRCS=	rump_cygwin_compat.c rump_cygwin_sysent.c
SRCS+=	sys_cygwin_component.c

# XXX
CPPFLAGS+= -I${.CURDIR} -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

SYS_P=rump_cygwin_sys
.include "../Makefile.sys"
