#	$NetBSD: Makefile,v 1.16 2013/06/06 00:54:49 dholland Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

.include "Makefile.inc"

PROG=	lfs_cleanerd
MAN=	lfs_cleanerd.8

# XXX
CPPFLAGS+=-I${NETBSDSRCDIR}/sys

BINDIR=		/libexec
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=      -static
.endif

.include <bsd.prog.mk>
