#	$NetBSD: Makefile,v 1.6 2019/05/01 08:53:00 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+= -DVND_COMPRESSION

.PATH:	${S}/dev

KMOD=	vnd
IOCONF=	vnd.ioconf
SRCS=	vnd.c

WARNS=	3

.include <bsd.kmodule.mk>
