#	$NetBSD: Makefile,v 1.1 2009/11/02 10:03:56 plunky Exp $

.include <bsd.init.mk>

LIB=		event
USE_SHLIBDIR=	yes

SRCS=		buffer.c evbuffer.c evdns.c event.c event_tagging.c \
		evrpc.c evutil.c http.c kqueue.c log.c poll.c select.c \
		signal.c

CPPFLAGS+=	-fno-strict-aliasing

CPPFLAGS.evdns.c+=		-Wno-shadow -Wno-cast-qual
CPPFLAGS.event_tagging.c+=	-Wno-cast-qual
CPPFLAGS.evrpc.c+=		-Wno-cast-qual
CPPFLAGS.http.c+=		-Wno-cast-qual -Wno-sign-compare
CPPFLAGS.kqueue.c+=		-Wno-sign-compare
CPPFLAGS.select.c+=		-Wno-sign-compare

LINTFLAGS+=	-X 27	# http.c "redeclaration of %s"

MAN=		evdns.3 event.3

MLINKS+=	event.3 event_init.3
MLINKS+=	event.3 event_dispatch.3
MLINKS+=	event.3 event_loop.3
MLINKS+=	event.3 event_loopexit.3
MLINKS+=	event.3 event_loopbreak.3
MLINKS+=	event.3 event_base_dispatch.3
MLINKS+=	event.3 event_base_free.3
MLINKS+=	event.3 event_base_loop.3
MLINKS+=	event.3 event_base_loopexit.3
MLINKS+=	event.3 event_base_loopbreak.3
MLINKS+=	event.3 event_base_set.3
MLINKS+=	event.3 event_set.3
MLINKS+=	event.3 event_add.3
MLINKS+=	event.3 event_del.3
MLINKS+=	event.3 event_once.3
MLINKS+=	event.3 event_base_once.3
MLINKS+=	event.3 event_pending.3
MLINKS+=	event.3 event_initialized.3

MLINKS+=	event.3 evtimer_set.3
MLINKS+=	event.3 evtimer_add.3
MLINKS+=	event.3 evtimer_del.3
MLINKS+=	event.3 evtimer_pending.3
MLINKS+=	event.3 evtimer_initialized.3

MLINKS+=	event.3 signal_set.3
MLINKS+=	event.3 signal_add.3
MLINKS+=	event.3 signal_del.3
MLINKS+=	event.3 signal_pending.3
MLINKS+=	event.3 signal_initialized.3

MLINKS+=	event.3 bufferevent_new.3
MLINKS+=	event.3 bufferevent_free.3
MLINKS+=	event.3 bufferevent_write.3
MLINKS+=	event.3 bufferevent_write_buffer.3
MLINKS+=	event.3 bufferevent_read.3
MLINKS+=	event.3 bufferevent_enable.3
MLINKS+=	event.3 bufferevent_disable.3
MLINKS+=	event.3 bufferevent_settimeout.3
MLINKS+=	event.3 bufferevent_base_set.3

MLINKS+=	event.3 evbuffer_new.3
MLINKS+=	event.3 evbuffer_free.3
MLINKS+=	event.3 evbuffer_add.3
MLINKS+=	event.3 evbuffer_add_buffer.3
MLINKS+=	event.3 evbuffer_add_printf.3
MLINKS+=	event.3 evbuffer_add_vprintf.3
MLINKS+=	event.3 evbuffer_drain.3
MLINKS+=	event.3 evbuffer_write.3
MLINKS+=	event.3 evbuffer_read.3
MLINKS+=	event.3 evbuffer_find.3
MLINKS+=	event.3 evbuffer_readline.3

MLINKS+=	event.3 evhttp_new.3
MLINKS+=	event.3 evhttp_bind_socket.3
MLINKS+=	event.3 evhttp_free.3

MLINKS+=	evdns.3 evdns_init.3
MLINKS+=	evdns.3 evdns_shutdown.3
MLINKS+=	evdns.3 evdns_err_to_string.3
MLINKS+=	evdns.3 evdns_nameserver_add.3
MLINKS+=	evdns.3 evdns_count_nameservers.3
MLINKS+=	evdns.3 evdns_clear_nameservers_and_suspend.3
MLINKS+=	evdns.3 evdns_resume.3
MLINKS+=	evdns.3 evdns_nameserver_ip_add.3
MLINKS+=	evdns.3 evdns_resolve_ipv4.3
MLINKS+=	evdns.3 evdns_resolve_reverse.3
MLINKS+=	evdns.3 evdns_resolv_conf_parse.3
MLINKS+=	evdns.3 evdns_search_clear.3
MLINKS+=	evdns.3 evdns_search_add.3
MLINKS+=	evdns.3 evdns_search_ndots_set.3
MLINKS+=	evdns.3 evdns_set_log_fn.3

.include <bsd.lib.mk>
