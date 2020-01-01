#	$NetBSD: Makefile,v 1.1 2020/01/01 10:13:16 ryo Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_aq
IOCONF=	if_aq.ioconf

SRCS=	if_aq.c

CPPFLAGS+=	-DINET -DNVLAN=1

.include <bsd.kmodule.mk>
