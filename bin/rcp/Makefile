#	$NetBSD: Makefile,v 1.16 1999/07/20 09:35:19 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

SRCTOP=	../..
.include <bsd.crypto.mk>

PROG=	rcp
SRCS=	rcp.c util.c

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/bin/rcp/Makefile.frag"
.endif

.include <bsd.prog.mk>
