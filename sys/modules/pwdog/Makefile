#	$NetBSD: Makefile,v 1.1 2011/08/11 07:33:25 mbalmer Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/pci

KMOD=		pwdog
IOCONF=		pwdog.ioconf
SRCS=		pwdog.c

CPPFLAGS+=	-I${S}/pci

.include <bsd.kmodule.mk>

