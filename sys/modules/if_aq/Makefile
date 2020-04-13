#	$NetBSD: Makefile,v 1.1.8.2 2020/04/13 08:05:11 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_aq
IOCONF=	if_aq.ioconf

SRCS=	if_aq.c

CPPFLAGS+=	-DINET -DNVLAN=1

.include <bsd.kmodule.mk>
