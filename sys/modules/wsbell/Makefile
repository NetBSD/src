#	$NetBSD: Makefile,v 1.1.6.2 2017/12/03 11:39:02 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/wscons

CPPFLAGS+=	-DNWSMUX=1

KMOD=	wsbell
IOCONF=	wsbell.ioconf
SRCS=	wsbell.c

.include <bsd.kmodule.mk>
