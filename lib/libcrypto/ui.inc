#	$NetBSD: ui.inc,v 1.2 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ui/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ui


UI_SRCS = ui_err.c ui_lib.c ui_openssl.c ui_util.c $(COMPATSRC)
SRCS += ${UI_SRCS}

.for cryptosrc in ${UI_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/ui
.endfor
