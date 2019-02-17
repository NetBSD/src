#	$NetBSD: Makefile,v 1.5 2019/02/17 04:06:00 rin Exp $

.include "../Makefile.inc"

CPPFLAGS+= -DVND_COMPRESSION -DCOMPAT_50

.PATH:	${S}/dev

KMOD=	vnd
IOCONF=	vnd.ioconf
SRCS=	vnd.c

WARNS=	3

.include <bsd.kmodule.mk>
