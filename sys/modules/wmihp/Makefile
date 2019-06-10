# $NetBSD: Makefile,v 1.3.58.1 2019/06/10 22:09:44 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmihp
IOCONF=	wmihp.ioconf
SRCS=	wmi_hp.c

.include <bsd.kmodule.mk>
