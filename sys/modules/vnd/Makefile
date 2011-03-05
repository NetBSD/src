#	$NetBSD: Makefile,v 1.1.10.1 2011/03/05 20:55:49 rmind Exp $

.include "../Makefile.inc"

CPPFLAGS+= -DVND_COMPRESSION -DCOMPAT_30 -DCOMPAT_50

.PATH:	${S}/dev


KMOD=	vnd
SRCS=	vnd.c

.include <bsd.kmodule.mk>
