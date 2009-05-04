#	$NetBSD: Makefile,v 1.1.6.2 2009/05/04 08:14:14 yamt Exp $

.include "../Makefile.inc"

CPPFLAGS+= -D VND_COMPRESSION

.PATH:	${S}/dev


KMOD=	vnd
SRCS=	vnd.c

.include <bsd.kmodule.mk>
