# $NetBSD: Makefile.inc,v 1.6 2001/07/14 03:17:44 matt Exp $

.if (${MACHINE_ARCH} == "arm32") || (${MACHINE_ARCH} == "arm")
SRCS+=	arm32_sync_icache.c arm32_drain_writebuf.c
.endif

MAN+=	arm32_sync_icache.2 arm32_drain_writebuf.2
