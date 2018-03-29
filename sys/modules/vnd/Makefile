#	$NetBSD: Makefile,v 1.3.16.1 2018/03/29 11:22:23 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+= -DVND_COMPRESSION -DCOMPAT_50

.PATH:	${S}/dev


KMOD=	vnd
IOCONF=	vnd.ioconf
SRCS=	vnd.c

.include <bsd.kmodule.mk>
