#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.8 1999/07/12 22:05:42 thorpej Exp $

SRCTOP=	../..
.include <bsd.crypto.mk>

LIB=	telnet
SRCS=	auth.c encrypt.c genget.c getent.c misc.c
CPPFLAGS+= -DHAS_CGETENT

.if defined(CRYPTOPATH)
.include "${CRYPTOPATH}/lib/libtelnet/Makefile.frag"
.endif

.include <bsd.lib.mk>
