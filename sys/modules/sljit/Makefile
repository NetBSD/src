# $NetBSD: Makefile,v 1.2.2.2 2012/10/30 17:22:42 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/sljit/sljit
.PATH:	${S}/external/bsd/sljit/dist/sljit_src

KMOD=	sljit
SRCS=	sljitLir.c sljit_mod.c

.include <bsd.kmodule.mk>
