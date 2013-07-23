# $NetBSD: Makefile.inc,v 1.8.66.1 2013/07/23 21:07:28 riastradh Exp $

.if ${MACHINE_CPU} == "m68k"
NOLINT=		# defined
SRCS=		m68k_sync_icache.S
.endif

MAN+=		m68k_sync_icache.2
