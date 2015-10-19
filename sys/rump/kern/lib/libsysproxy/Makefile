#	$NetBSD: Makefile,v 1.2 2015/10/19 16:16:37 pooka Exp $
#

LIB=	rumpkern_sysproxy
COMMENT=Remote system call support (rump kernel as a server)

SRCS=	sysproxy.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
