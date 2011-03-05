#	$NetBSD: Makefile,v 1.1.2.2 2011/03/05 15:10:44 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_alc
IOCONF=	if_alc.ioconf

SRCS=	if_alc.c

WARNS=	3

CPPFLAGS+=	-DINET -DNVLAN=1

.include <bsd.kmodule.mk>
