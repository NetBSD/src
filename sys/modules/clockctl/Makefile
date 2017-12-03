# $NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:50 jdolecek Exp $

.include "../Makefile.inc"

.PATH: ${S}/dev

KMOD=		clockctl
IOCONF=		clockctl.ioconf
SRCS=		clockctl.c

CPPFLAGS+=	-DNTP -DCOMPAT_50

.include <bsd.kmodule.mk>
