# $NetBSD: Makefile.inc,v 1.7 2002/03/26 22:48:04 fredette Exp $

.if (${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "m68000")
SRCS=		m68k_sync_icache.S
NOLINT=		# defined
.endif

MAN+=		m68k_sync_icache.2
