#	$NetBSD: bf.inc,v 1.4 2001/09/09 21:22:09 tls Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/bf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bf

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/bf

.if (${MACHINE_CPU} == "i386")
CPPFLAGS+=      -DBF_PTR2
.endif
.if (${MACHINE_CPU} == "sparc" || ${MACHINE_CPU} == "mips") 
CPPFLAGS+=      -DBF_PTR
.endif

SRCS+=bf_skey.c bf_ecb.c   bf_enc.c  bf_cfb64.c bf_ofb64.c


