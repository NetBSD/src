#	$NetBSD: md5.inc,v 1.5 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/md5/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/md5


MD5_SRCS = md5_dgst.c md5_one.c $(MD5_ASM_OBJ)
SRCS += ${MD5_SRCS}

.for cryptosrc in ${MD5_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/md5
.endfor
