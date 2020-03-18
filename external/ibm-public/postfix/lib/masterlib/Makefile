#	$NetBSD: Makefile,v 1.3 2020/03/18 19:05:22 christos Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	master

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS= \
dgram_server.c \
event_server.c \
mail_flow.c \
master_avail.c \
master_conf.c \
master_flow.c \
master_listen.c \
master_monitor.c \
master_proto.c \
master_service.c \
master_sig.c \
master_spawn.c \
master_status.c \
master_wakeup.c \
master_watch.c \
multi_server.c \
single_server.c \
trigger_server.c

.include <bsd.lib.mk>
