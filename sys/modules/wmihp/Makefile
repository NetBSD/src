# $NetBSD: Makefile,v 1.4 2019/02/17 04:06:01 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmihp
IOCONF=	wmihp.ioconf
SRCS=	wmi_hp.c

.include <bsd.kmodule.mk>
