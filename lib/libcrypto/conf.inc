#	$NetBSD: conf.inc,v 1.6 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/conf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/conf


CONF_SRCS = conf_err.c conf_lib.c conf_api.c conf_def.c conf_mod.c \
	conf_mall.c conf_sap.c
SRCS += ${CONF_SRCS}

.for cryptosrc in ${CONF_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/conf
.endfor
