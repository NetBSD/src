# $NetBSD: Makefile.inc,v 1.4 1999/06/10 00:49:51 simonb Exp $

.if (${MACHINE_ARCH} == "arm32")
.PATH: ${LIBC}/arm32

SRCS+=	arm32_sync_icache.c arm32_drain_writebuf.c
.endif

MAN+=	arm32_sync_icache.2 arm32_drain_writebuf.2
