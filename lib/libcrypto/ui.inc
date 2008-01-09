#	$NetBSD: ui.inc,v 1.2.10.1 2008/01/09 01:34:41 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/ui/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/ui


UI_SRCS = ui_err.c ui_lib.c ui_openssl.c ui_util.c $(COMPATSRC)
SRCS += ${UI_SRCS}

.for cryptosrc in ${UI_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/ui
.endfor
