#	$NetBSD: Makefile,v 1.15 2009/08/06 00:51:55 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

.include "Makefile.inc"

PROG=	lfs_cleanerd
MAN=	lfs_cleanerd.8

BINDIR=		/libexec
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=      -static
.endif

.include <bsd.prog.mk>
