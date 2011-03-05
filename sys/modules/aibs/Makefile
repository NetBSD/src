#	$NetBSD: Makefile,v 1.3.2.2 2011/03/05 20:55:37 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	aibs
IOCONF=	aibs.ioconf
SRCS=	atk0110.c

WARNS=	4

.include <bsd.kmodule.mk>
