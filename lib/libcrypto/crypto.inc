#	$NetBSD: crypto.inc,v 1.2 2000/06/21 19:34:24 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto

CPPFLAGS+=	-I${OPENSSLSRC}/crypto

SRCS+= cryptlib.c mem.c cversion.c ex_data.c tmdiff.c cpt_err.c


