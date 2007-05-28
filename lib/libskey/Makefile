#	$NetBSD: Makefile,v 1.6 2007/05/28 12:06:21 tls Exp $

USE_FORT?= yes	# authentication software

LIB=	skey
SRCS=	skeylogin.c skeysubr.c put.c
INCS=	skey.h
INCSDIR=/usr/include

MAN=	skey.3

MLINKS=	skey.3 skeychallenge.3 \
	skey.3 skeylookup.3 \
	skey.3 skeygetnext.3 \
	skey.3 skeyverify.3 \
	skey.3 skeyzero.3 \
	skey.3 getskeyprompt.3 \
	skey.3 skey_set_algorithm.3 \
	skey.3 skey_get_algorithm.3 \
	skey.3 skey_haskey.3 \
	skey.3 skey_keyinfo.3 \
	skey.3 skey_passcheck.3 \
	skey.3 skey_authenticate.3

.include <bsd.lib.mk>
