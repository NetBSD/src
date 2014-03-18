# $NetBSD: Makefile.inc,v 1.10 2014/03/18 18:20:37 riastradh Exp $

.if ${MACHINE_CPU} == "m68k"
NOLINT=		# defined
SRCS=		m68k_sync_icache.S
.endif

MAN+=		m68k_sync_icache.2
