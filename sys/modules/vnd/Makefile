#	$NetBSD: Makefile,v 1.1.8.1 2010/10/22 07:22:38 uebayasi Exp $

.include "../Makefile.inc"

CPPFLAGS+= -DVND_COMPRESSION -DCOMPAT_30 -DCOMPAT_50

.PATH:	${S}/dev


KMOD=	vnd
SRCS=	vnd.c

.include <bsd.kmodule.mk>
