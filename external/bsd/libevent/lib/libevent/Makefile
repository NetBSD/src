# $NetBSD: Makefile,v 1.6 2023/06/03 21:30:20 lukem Exp $

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

COPTS.bufferevent_filter.c +=	${CC_WNO_IMPLICIT_FALLTHROUGH}
COPTS.evdns.c +=		${CC_WNO_IMPLICIT_FALLTHROUGH}
COPTS.event.c +=		${CC_WNO_IMPLICIT_FALLTHROUGH}

.include <bsd.lib.mk>
