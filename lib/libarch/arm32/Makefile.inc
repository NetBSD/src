# $NetBSD: Makefile.inc,v 1.3 1999/06/09 05:43:10 simonb Exp $

.PATH: ${LIBC}/arm32

SRCS+=	arm32_sync_icache.c arm32_drain_writebuf.c

MAN+=	arm32_sync_icache.2 arm32_drain_writebuf.2

.include <bsd.lib.mk>

