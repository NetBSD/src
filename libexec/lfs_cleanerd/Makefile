#	$NetBSD: Makefile,v 1.19 2015/10/10 22:34:46 dholland Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

.include "Makefile.inc"

PROG=	lfs_cleanerd
MAN=	lfs_cleanerd.8

BINDIR=		/libexec
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=      -static
.endif

.include <bsd.prog.mk>
