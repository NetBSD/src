# $Header: /cvsroot/src/lib/libcrypt/Makefile,v 1.2 1993/05/29 13:27:15 cgd Exp $
#
# This directory contains an implementation of crypt(3) and associated
# routines.  the file crypt.c can't be shipped out of the US.  it was put
# into this directory to make distribution of exportable and non-exportable
# systems easier.

.if exists(crypt.c)
LIB=	crypt
SRCS=	crypt.c
	 
.include <bsd.lib.mk>
.else
all install clean cleandir depend obj:
.endif
