#	from: @(#)Makefile	5.5 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.4 1993/07/30 23:50:50 mycroft Exp $

PROG=	su
BINOWN=	root
BINMODE=4555
.ifndef EXPORTABLE_SYSTEM
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
.endif

.include <bsd.prog.mk>
