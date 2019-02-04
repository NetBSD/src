# $NetBSD: Makefile,v 1.5 2019/02/04 04:05:16 mrg Exp $

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

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.bufferevent_filter.c += -Wno-error=implicit-fallthrough
COPTS.evdns.c += -Wno-error=implicit-fallthrough
COPTS.event.c += -Wno-error=implicit-fallthrough
.endif

.include <bsd.lib.mk>
