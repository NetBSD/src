#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:04 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	cac
IOCONF=	cac.ioconf

SRCS+=	cac.c

CPPFLAGS+=	-DNBIO=1

.include <bsd.kmodule.mk>
