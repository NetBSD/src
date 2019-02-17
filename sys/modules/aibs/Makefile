#	$NetBSD: Makefile,v 1.5 2019/02/17 04:05:42 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	aibs
IOCONF=	aibs.ioconf
SRCS=	aibs_acpi.c

.include <bsd.kmodule.mk>
