# $NetBSD: Makefile.inc,v 1.2 2014/08/10 23:25:50 matt Exp $

.include <bsd.own.mk>

LIBC_MACHINE_CPU?=	${MACHINE_CPU}

.if (${LIBC_MACHINE_CPU} == "arm")
SRCS+=	arm_sync_icache.c arm_drain_writebuf.c
.endif

MAN+=	arm_sync_icache.2 arm_drain_writebuf.2
