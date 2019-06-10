#	$NetBSD: Makefile,v 1.3.18.1 2019/06/10 22:09:43 christos Exp $

.include "../Makefile.inc"

CPPFLAGS+= -DVND_COMPRESSION

.PATH:	${S}/dev

KMOD=	vnd
IOCONF=	vnd.ioconf
SRCS=	vnd.c

WARNS=	3

.include <bsd.kmodule.mk>
