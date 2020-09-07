#	$NetBSD: Makefile,v 1.21 2020/09/07 03:09:53 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

.include "Makefile.inc"

PROG=	lfs_cleanerd
MAN=	lfs_cleanerd.8

BINDIR=		/libexec
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=      -static
.endif

.include <bsd.prog.mk>
