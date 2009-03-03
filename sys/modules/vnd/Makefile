#	$NetBSD: Makefile,v 1.1.4.2 2009/03/03 18:33:38 skrll Exp $

.include "../Makefile.inc"

CPPFLAGS+= -D VND_COMPRESSION

.PATH:	${S}/dev


KMOD=	vnd
SRCS=	vnd.c

.include <bsd.kmodule.mk>
