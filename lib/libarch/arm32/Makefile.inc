# $NetBSD: Makefile.inc,v 1.5 1999/08/31 13:50:47 simonb Exp $

.if (${MACHINE_ARCH} == "arm32")
SRCS+=	arm32_sync_icache.c arm32_drain_writebuf.c
.endif

MAN+=	arm32_sync_icache.2 arm32_drain_writebuf.2
