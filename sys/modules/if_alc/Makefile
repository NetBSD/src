#	$NetBSD: Makefile,v 1.1.8.2 2011/06/06 09:09:48 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_alc
IOCONF=	if_alc.ioconf

SRCS=	if_alc.c

WARNS=	3

CPPFLAGS+=	-DINET -DNVLAN=1

.include <bsd.kmodule.mk>
