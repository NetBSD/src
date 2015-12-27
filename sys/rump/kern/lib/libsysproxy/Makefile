#	$NetBSD: Makefile,v 1.1.2.3 2015/12/27 12:10:16 skrll Exp $
#

LIB=	rumpkern_sysproxy
COMMENT=Remote system call support (rump kernel as a server)

SRCS=	sysproxy.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
