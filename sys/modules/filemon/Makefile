# $NetBSD: Makefile,v 1.1.24.1 2017/12/03 11:38:54 jdolecek Exp $

.include "../Makefile.inc"

.PATH: ${S}/dev/filemon

KMOD = filemon
IOCONF=	filemon.ioconf
SRCS = filemon.c filemon_wrapper.c
NOMAN = no

.include <bsd.kmodule.mk>
