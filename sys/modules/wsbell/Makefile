#	$NetBSD: Makefile,v 1.1.4.2 2017/08/28 17:53:11 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/wscons

CPPFLAGS+=	-DNWSMUX=1

KMOD=	wsbell
IOCONF=	wsbell.ioconf
SRCS=	wsbell.c

.include <bsd.kmodule.mk>
