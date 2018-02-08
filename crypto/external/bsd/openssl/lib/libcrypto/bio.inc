#	$NetBSD: bio.inc,v 1.2 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/bio/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/bio


BIO_SRCS = \
b_addr.c \
b_dump.c \
b_print.c \
b_sock.c \
b_sock2.c \
bf_buff.c \
bf_lbuf.c \
bf_nbio.c \
bf_null.c \
bio_cb.c \
bio_err.c \
bio_lib.c \
bio_meth.c \
bss_acpt.c \
bss_bio.c \
bss_conn.c \
bss_dgram.c \
bss_fd.c \
bss_file.c \
bss_log.c \
bss_mem.c \
bss_null.c \
bss_sock.c
SRCS += ${BIO_SRCS}

.for cryptosrc in ${BIO_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/bio
.endfor
