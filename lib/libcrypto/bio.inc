#	$NetBSD: bio.inc,v 1.6 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/bio/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bio


BIO_SRCS = bio_lib.c bio_cb.c bio_err.c \
	bss_mem.c bss_null.c bss_fd.c \
	bss_file.c bss_sock.c bss_conn.c \
	bf_null.c bf_buff.c b_print.c b_dump.c \
	b_sock.c bss_acpt.c bf_nbio.c bss_log.c bss_bio.c bss_dgram.c
SRCS += ${BIO_SRCS}

.for cryptosrc in ${BIO_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/bio
.endfor
