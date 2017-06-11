#	$NetBSD: Makefile,v 1.1 2017/06/11 22:21:33 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/wscons

CPPFLAGS+=	-DNWSMUX=1

KMOD=	wsbell
IOCONF=	wsbell.ioconf
SRCS=	wsbell.c

.include <bsd.kmodule.mk>
