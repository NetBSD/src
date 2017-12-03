#	$NetBSD: Makefile,v 1.1.4.4 2017/12/03 11:39:15 jdolecek Exp $
#

LIB=	rumpkern_sys_cygwin
COMMENT=Cygwin system call translation

SRCS=	rump_cygwin_compat.c
SRCS+=	sys_cygwin_component.c

SYS_P=rump_cygwin_sys
.include "../Makefile.sys"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
