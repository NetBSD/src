#	$NetBSD: Makefile,v 1.1 2009/02/05 17:32:10 haad Exp $

.include "../Makefile.inc"

CPPFLAGS+= -D VND_COMPRESSION

.PATH:	${S}/dev


KMOD=	vnd
SRCS=	vnd.c

.include <bsd.kmodule.mk>
