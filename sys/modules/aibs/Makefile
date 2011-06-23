#	$NetBSD: Makefile,v 1.3.4.1 2011/06/23 14:20:25 cherry Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	aibs
IOCONF=	aibs.ioconf
SRCS=	aibs_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
