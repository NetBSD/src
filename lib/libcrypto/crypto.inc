#	$NetBSD: crypto.inc,v 1.5 2003/07/24 14:16:32 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto

CPPFLAGS+=	-I${OPENSSLSRC}/crypto

SRCS+=	cryptlib.c mem.c mem_clr.c mem_dbg.c cversion.c ex_data.c tmdiff.c \
	cpt_err.c ebcdic.c uid.c o_time.c
