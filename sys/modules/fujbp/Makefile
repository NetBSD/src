# $NetBSD: Makefile,v 1.1.4.2 2011/03/05 20:55:41 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	fujbp
IOCONF=	fujbp.ioconf
SRCS=	fujbp_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
