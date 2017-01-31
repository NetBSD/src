# $NetBSD: Makefile,v 1.3 2017/01/31 23:17:40 christos Exp $

USE_SHLIBDIR=yes
.include <bsd.own.mk>

LIB=event

SRCS= \
buffer.c \
bufferevent.c \
bufferevent_filter.c \
bufferevent_pair.c \
bufferevent_ratelim.c \
bufferevent_sock.c \
evdns.c \
event.c \
event_tagging.c \
evmap.c \
evrpc.c \
evthread.c \
evutil.c \
evutil_rand.c \
evutil_time.c \
http.c \
kqueue.c \
listener.c \
log.c \
poll.c \
select.c \
signal.c

.if defined(HAVE_GCC) && ${HAVE_GCC} == 48
.if ${MACHINE_ARCH} == "sh3el"
COPTS.evdns.c=	-O0
.endif
.endif

.include <bsd.lib.mk>
