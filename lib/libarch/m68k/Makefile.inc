# $NetBSD: Makefile.inc,v 1.8.54.1 2014/05/22 11:36:46 yamt Exp $

.if ${MACHINE_CPU} == "m68k"
NOLINT=		# defined
SRCS=		m68k_sync_icache.S
.endif

MAN+=		m68k_sync_icache.2
