# This directory contains an implementation of crypt(3) and associated
# routines.  the file crypt.c can't be shipped out of the US.  it was put
# into this directory to make distribution of exportable and non-exportable
# systems easier.
#
#	$Id: Makefile,v 1.5 1994/12/22 09:49:34 cgd Exp $

LIB=	crypt

.if exists(crypt.c) && !defined(EXPORTABLE_SYSTEM)
SRCS=	crypt.c
.else
SRCS=	crypt_dummy.c
.endif

MAN=	crypt.3
MLINKS= crypt.3 encrypt.3 crypt.3 setkey.3

.include <bsd.lib.mk>
