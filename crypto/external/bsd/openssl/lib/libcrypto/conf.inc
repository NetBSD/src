#	$NetBSD: conf.inc,v 1.2 2018/08/18 16:40:02 rjs Exp $
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
