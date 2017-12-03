#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:50 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	cac
IOCONF=	cac.ioconf

SRCS+=	cac.c

CPPFLAGS+=	-DNBIO=1

.include <bsd.kmodule.mk>
