#	$NetBSD: Makefile,v 1.15 1999/07/12 22:04:21 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

SRCTOP=	../..
.include <bsd.crypto.mk>

PROG=	rcp
SRCS=	rcp.c util.c

.if defined(CRYPTOPATH)
.include "${CRYPTOPATH}/bin/rcp/Makefile.frag"
.endif

.include <bsd.prog.mk>
