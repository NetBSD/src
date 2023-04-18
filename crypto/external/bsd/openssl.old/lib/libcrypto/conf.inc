#	$NetBSD: conf.inc,v 1.1.1.2 2023/04/18 14:19:03 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/conf/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/conf


CONF_SRCS = conf_err.c conf_lib.c conf_api.c conf_def.c conf_mod.c \
	conf_mall.c conf_sap.c conf_ssl.c
SRCS += ${CONF_SRCS}

.for cryptosrc in ${CONF_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/conf
.endfor
