#	$NetBSD: Makefile,v 1.4.8.1 2025/08/02 05:50:21 perseant Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	tls

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS=	 \
tls_bio_ops.c \
tls_certkey.c \
tls_client.c \
tls_dane.c \
tls_dh.c \
tls_fprint.c \
tls_level.c \
tls_mgr.c \
tls_misc.c \
tls_prng_dev.c \
tls_prng_egd.c \
tls_prng_exch.c \
tls_prng_file.c \
tls_proxy_client_misc.c \
tls_proxy_client_print.c \
tls_proxy_client_scan.c \
tls_proxy_clnt.c \
tls_proxy_context_print.c \
tls_proxy_context_scan.c \
tls_proxy_server_print.c \
tls_proxy_server_scan.c \
tls_scache.c \
tls_seed.c \
tls_server.c \
tls_session.c \
tls_stream.c \
tls_verify.c \
tlsrpt_wrapper.c

.include <bsd.lib.mk>
