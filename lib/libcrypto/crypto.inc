#	$NetBSD: crypto.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto

CFLAGS+=	-I${OPENSSLSRC}/crypto

SRCS+= cryptlib.c mem.c cversion.c ex_data.c tmdiff.c cpt_err.c


