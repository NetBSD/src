# $NetBSD: Makefile.inc,v 1.5 1999/06/10 00:49:52 simonb Exp $

.if (${MACHINE_ARCH} == "m68k")
SRCS=	m68k_sync_icache.S
MKLINT=no
.endif

MAN+=	m68k_sync_icache.2
