#	from: @(#)Makefile	5.5 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.5 1993/10/07 02:20:19 cgd Exp $

PROG=	su
BINOWN=	root
BINMODE=4555
LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

.include <bsd.prog.mk>
