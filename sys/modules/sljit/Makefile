# $NetBSD: Makefile,v 1.2 2012/10/21 21:23:38 alnsn Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/sljit/sljit
.PATH:	${S}/external/bsd/sljit/dist/sljit_src

KMOD=	sljit
SRCS=	sljitLir.c sljit_mod.c

.include <bsd.kmodule.mk>
