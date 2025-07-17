#	$NetBSD: quic.inc,v 1.1 2025/07/17 14:26:12 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/ssl/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/ssl/quic

CPPFLAGS+=	-I${OPENSSLSRC}/ssl/quic

SRCS+= \
cc_newreno.c \
json_enc.c \
qlog.c \
qlog_event_helpers.c \
quic_ackm.c \
quic_cfq.c \
quic_channel.c \
quic_demux.c \
quic_engine.c \
quic_fc.c \
quic_fifd.c \
quic_impl.c \
quic_lcidm.c \
quic_method.c \
quic_obj.c \
quic_port.c \
quic_rcidm.c \
quic_reactor.c \
quic_reactor_wait_ctx.c \
quic_record_rx.c \
quic_record_shared.c \
quic_record_tx.c \
quic_record_util.c \
quic_rstream.c \
quic_rx_depack.c \
quic_sf_list.c \
quic_srt_gen.c \
quic_srtm.c \
quic_sstream.c \
quic_statm.c \
quic_stream_map.c \
quic_thread_assist.c \
quic_tls.c \
quic_tls_api.c \
quic_trace.c \
quic_tserver.c \
quic_txp.c \
quic_txpim.c \
quic_types.c \
quic_wire.c \
quic_wire_pkt.c \
uint_set.c \
