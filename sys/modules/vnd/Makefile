#	$NetBSD: Makefile,v 1.4 2019/01/27 02:08:48 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+= -DVND_COMPRESSION -DCOMPAT_50

.PATH:	${S}/dev


KMOD=	vnd
IOCONF=	vnd.ioconf
SRCS=	vnd.c

.include <bsd.kmodule.mk>
