#	$NetBSD: bio.inc,v 1.2.2.2 2000/06/23 16:16:21 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/bio/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bio

CFLAGS+=	-I${OPENSSLSRC}/crypto/bio

SRCS+= bio_lib.c bio_cb.c bio_err.c \
	bss_mem.c bss_null.c bss_fd.c \
	bss_file.c bss_sock.c bss_conn.c \
	bf_null.c bf_buff.c b_print.c b_dump.c \
	b_sock.c bss_acpt.c bf_nbio.c bss_log.c


