# $NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:22 christos Exp $

.include "../Makefile.inc"

.PATH: ${S}/kern

KMOD=		drvctl
IOCONF=		drvctl.ioconf
SRCS=		kern_drvctl.c

WARNS=		3

.include <bsd.kmodule.mk>
