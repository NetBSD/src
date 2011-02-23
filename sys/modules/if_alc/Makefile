#	$NetBSD: Makefile,v 1.1 2011/02/23 00:36:51 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_alc
IOCONF=	if_alc.ioconf

SRCS=	if_alc.c

WARNS=	3

CPPFLAGS+=	-DINET -DNVLAN=1

.include <bsd.kmodule.mk>
