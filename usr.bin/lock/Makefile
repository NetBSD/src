#	from: @(#)Makefile	5.5 (Berkeley) 4/16/91
#	$Id: Makefile,v 1.5 1993/10/07 02:18:49 cgd Exp $

PROG=	lock
BINOWN= root
BINMODE=4555
LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

.include <bsd.prog.mk>
