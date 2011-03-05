# $NetBSD: Makefile,v 1.1.4.2 2011/03/05 20:55:42 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	fujhk
IOCONF=	fujhk.ioconf
SRCS=	fujhk_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
