# $NetBSD: Makefile.inc,v 1.1 2002/03/30 06:24:34 thorpej Exp $

.include <bsd.own.mk>

.if (${MACHINE_CPU} == "arm")
SRCS+=	arm_sync_icache.c arm_drain_writebuf.c
.endif

MAN+=	arm_sync_icache.2 arm_drain_writebuf.2
