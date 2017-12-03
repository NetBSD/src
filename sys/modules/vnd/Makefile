#	$NetBSD: Makefile,v 1.2.18.1 2017/12/03 11:39:01 jdolecek Exp $

.include "../Makefile.inc"

CPPFLAGS+= -DVND_COMPRESSION -DCOMPAT_30 -DCOMPAT_50

.PATH:	${S}/dev


KMOD=	vnd
IOCONF=	vnd.ioconf
SRCS=	vnd.c

.include <bsd.kmodule.mk>
