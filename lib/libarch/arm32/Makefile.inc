# $NetBSD: Makefile.inc,v 1.7 2001/07/28 19:17:11 bjh21 Exp $

.include <bsd.own.mk>

.if (${MACHINE_CPU} == "arm")
SRCS+=	arm32_sync_icache.c arm32_drain_writebuf.c
.endif

MAN+=	arm32_sync_icache.2 arm32_drain_writebuf.2
