#	from: @(#)Makefile	5.5 (Berkeley) 4/16/91
#	$Id: Makefile,v 1.4 1993/07/31 15:20:41 mycroft Exp $

PROG=	lock
BINOWN= root
BINMODE=4555
.ifndef EXPORTABLE_SYSTEM
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
.endif

.include <bsd.prog.mk>
