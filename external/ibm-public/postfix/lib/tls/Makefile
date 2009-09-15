#	$NetBSD: Makefile,v 1.1.2.2 2009/09/15 06:04:14 snj Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	tls

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS=	tls_prng_dev.c tls_prng_egd.c tls_prng_file.c \
	tls_prng_exch.c tls_stream.c tls_bio_ops.c tls_misc.c tls_dh.c \
	tls_rsa.c tls_verify.c tls_certkey.c tls_session.c \
	tls_client.c tls_server.c tls_scache.c tls_mgr.c tls_seed.c \
	tls_level.c

.include <bsd.lib.mk>
