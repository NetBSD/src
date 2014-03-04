#	$NetBSD: Makefile,v 1.18 2014/03/04 21:06:47 joerg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

.include "Makefile.inc"

PROG=	lfs_cleanerd
MAN=	lfs_cleanerd.8

BINDIR=		/libexec
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=      -static
.endif

CWARNFLAGS.clang+=	-Wno-error=absolute-value

.include <bsd.prog.mk>
