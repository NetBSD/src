# $NetBSD: Makefile.inc,v 1.6 2001/12/12 00:51:00 tv Exp $

.if (${MACHINE_ARCH} == "m68k")
SRCS=		m68k_sync_icache.S
NOLINT=		# defined
.endif

MAN+=		m68k_sync_icache.2
