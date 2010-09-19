#	$NetBSD: Makefile,v 1.2 2010/09/19 05:42:10 mrg Exp $

.include "../Makefile.inc"

CPPFLAGS+= -DVND_COMPRESSION -DCOMPAT_30 -DCOMPAT_50

.PATH:	${S}/dev


KMOD=	vnd
SRCS=	vnd.c

.include <bsd.kmodule.mk>
