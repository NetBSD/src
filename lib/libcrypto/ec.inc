#	$NetBSD: ec.inc,v 1.1 2003/07/24 14:16:32 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ec/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ec

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/ec

SRCS+=	ec_lib.c ecp_smpl.c ecp_mont.c ecp_recp.c ecp_nist.c ec_cvt.c ec_mult.c \
	ec_err.c
