#	$NetBSD: bio.inc,v 1.3 2000/07/16 07:16:21 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/bio/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bio

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/bio

SRCS+= bio_lib.c bio_cb.c bio_err.c \
	bss_mem.c bss_null.c bss_fd.c \
	bss_file.c bss_sock.c bss_conn.c \
	bf_null.c bf_buff.c b_print.c b_dump.c \
	b_sock.c bss_acpt.c bf_nbio.c bss_log.c bss_bio.c


