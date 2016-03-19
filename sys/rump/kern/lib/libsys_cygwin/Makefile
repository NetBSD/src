#	$NetBSD: Makefile,v 1.2.8.4 2016/03/19 11:30:36 skrll Exp $
#

LIB=	rumpkern_sys_cygwin
COMMENT=Cygwin system call translation

SRCS=	rump_cygwin_compat.c
SRCS+=	sys_cygwin_component.c

SYS_P=rump_cygwin_sys
.include "../Makefile.sys"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
