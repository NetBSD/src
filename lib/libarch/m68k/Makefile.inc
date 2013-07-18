# $NetBSD: Makefile.inc,v 1.9 2013/07/18 22:14:10 matt Exp $

.if ${MACHINE_CPU} == "m68k"
NOLINT=		# defined
SRCS=		m68k_sync_icache.S
.endif

MAN+=		m68k_sync_icache.2
