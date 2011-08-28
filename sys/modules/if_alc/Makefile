#	$NetBSD: Makefile,v 1.2 2011/08/28 15:48:19 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_alc
IOCONF=	if_alc.ioconf

SRCS=	if_alc.c

CPPFLAGS+=	-DINET -DNVLAN=1

.include <bsd.kmodule.mk>
