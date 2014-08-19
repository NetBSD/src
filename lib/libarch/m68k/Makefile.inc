# $NetBSD: Makefile.inc,v 1.8.60.1 2014/08/20 00:02:08 tls Exp $

.if ${MACHINE_CPU} == "m68k"
NOLINT=		# defined
SRCS=		m68k_sync_icache.S
.endif

MAN+=		m68k_sync_icache.2
