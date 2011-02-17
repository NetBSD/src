#	$NetBSD: Makefile,v 1.2.6.1 2011/02/17 12:00:47 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	aibs
IOCONF=	aibs.ioconf
SRCS=	atk0110.c

WARNS=	4

.include <bsd.kmodule.mk>
