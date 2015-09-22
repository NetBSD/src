#	$NetBSD: Makefile,v 1.2.36.1 2015/09/22 12:06:10 skrll Exp $

.include "../Makefile.inc"

CPPFLAGS+= -DVND_COMPRESSION -DCOMPAT_30 -DCOMPAT_50

.PATH:	${S}/dev


KMOD=	vnd
IOCONF=	vnd.ioconf
SRCS=	vnd.c

.include <bsd.kmodule.mk>
