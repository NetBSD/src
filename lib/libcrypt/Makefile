# This directory contains an implementation of crypt(3) and associated
# routines.  the file crypt.c can't be shipped out of the US.  it was put
# into this directory to make distribution of exportable and non-exportable
# systems easier.
#
#	$Id: Makefile,v 1.3 1993/08/02 17:50:08 mycroft Exp $

.if exists(crypt.c)
LIB=	crypt
SRCS=	crypt.c
	 
.include <bsd.lib.mk>
.else
all install clean cleandir depend obj:
.endif
