#	$NetBSD: crypto.inc,v 1.1.1.1.2.2 2000/07/17 05:59:58 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto

CPPFLAGS+=	-I${OPENSSLSRC}/crypto

SRCS+= cryptlib.c mem.c mem_dbg.c cversion.c ex_data.c tmdiff.c cpt_err.c ebcdic.c


