#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.3 1993/08/01 05:39:12 mycroft Exp $

PROG=	makekey
MAN8=	makekey.0
.ifndef EXPORTABLE_SYSTEM
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
.endif

.include <bsd.prog.mk>
