#	$NetBSD: Makefile,v 1.6 2016/01/26 23:12:17 pooka Exp $
#

LIB=	rumpkern_sys_cygwin
COMMENT=Cygwin system call translation

SRCS=	rump_cygwin_compat.c
SRCS+=	sys_cygwin_component.c

SYS_P=rump_cygwin_sys
.include "../Makefile.sys"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
