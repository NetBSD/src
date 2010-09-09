# $NetBSD: Makefile,v 1.1 2010/09/09 00:10:16 sjg Exp $

.include "../Makefile.inc"

.PATH: ${S}/dev/filemon

KMOD = filemon
SRCS = filemon.c filemon_wrapper.c
NOMAN = no

.include <bsd.kmodule.mk>
